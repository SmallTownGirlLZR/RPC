#define THREADED 1
#include "include/ZookeeperUtil.h"
#include "include/RpcApplication.h"
#include <cstdlib>
#include <mutex>
#include "include/RpcLogger.h"
#include <condition_variable>
#include <zookeeper/zookeeper.h>
std::mutex cv_mutex;        // 全局锁，用于保护共享变量的线程安全
std::condition_variable cv; // 条件变量，用于线程间通信
bool is_connected = false;  // 标记ZooKeeper客户端是否连接成功

// 全局watcher 观察器 收取ZooKeeper服务器通知
void global_watcher(zhandle_t* zh, int type, int status, const char* path, void* watcherCtx) {
    /*
     * type == ZOO_SESSION_EVENT 会话级别的事件 代表连接状态变化
     * type == ZOO_CONNECTED_STATE 节点被创建
     * */
    if(type == ZOO_SESSION_EVENT) {
        /*
         * status == ZOO_CONNECTED_STATE 连接完毕 
         * status == ZOO_CONNECTING_STATE 正在连接
         * status == ZOO_EXPIRED_SESSION_STATE 会话过期 
         * */
        if(status == ZOO_CONNECTED_STATE) {     // ZooKeeper 客户端和服务器连接成功
            std::lock_guard<std::mutex> lock(cv_mutex);
            is_connected = true;
        }
    }
    cv.notify_all();        // 通知等待的线程
}

ZkClient::ZkClient() : m_zhandle(nullptr) {}

// 析构函数，关闭ZooKeeper连接
ZkClient::~ZkClient() {
    if (m_zhandle != nullptr) {
        zookeeper_close(m_zhandle);  // 关闭ZooKeeper连接
    }
}

void ZkClient::Start() {
   // 从配置文件中读取ZooKeeper服务器的IP和端口
    std::string host = KrpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = KrpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;  // 拼接连接字符串
                                              
    
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 6000, nullptr, nullptr, 0);
    if(m_zhandle == nullptr) {
        LOG(ERROR) << "ZooKeeper_init error";
        exit(EXIT_FAILURE);
    }
    // global_watcher 调用后唤醒线程 
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock, []{return is_connected;});       // 阻塞至连接成功

    LOG(INFO) << "zookeeper init success";  // 连接成功 
}


// 在服务器上创建节点
void ZkClient::Create(const char* path, const char* data, int datalen, int state) {
    char path_buffer[128];  // 储存创建的节点路径 
    int bufferlen = sizeof(path_buffer);
     
    int flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if(flag == ZNONODE) {
        flag = zoo_create(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (flag == ZOK) {  // 创建成功
            LOG(INFO) << "znode create success... path:" << path;
        } else {  // 创建失败
            LOG(ERROR) << "znode create failed... path:" << path;
            exit(EXIT_FAILURE);  // 退出程序
        }
    }
    
}


// 向服务器传入path 获取IP:PORT 
std::string ZkClient::GetData(const char* path) {
    char buf[64];
    int bufferlen = sizeof(buf);

    int flag = zoo_get(m_zhandle, path, 0, buf, &bufferlen, nullptr);
    if(flag != ZOK) {
        LOG(ERROR) << "zoo get error";
        return "";
    }else{
        return std::string(buf, bufferlen);
    }
    return "";
}



