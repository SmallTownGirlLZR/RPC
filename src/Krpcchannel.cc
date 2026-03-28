#include "Krpcheader.pb.h"
#include "include/Krpcchannel.h"
#include "include/zookeeperutil.h"
#include <cerrno>
#include <cstdint>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <vector>

std::mutex g_data_mutex;

Krpcchannel::Krpcchannel(bool connectNow)
    :m_clientfd(-1),
     m_idx(0){
    if (!connectNow) {  // 如果不需要立即连接
        return;
    }

    // 尝试连接服务器，最多重试3次
    auto rt = newConnect(m_ip.c_str(), m_port);
    int count = 3;  // 重试次数
    while (!rt && count--) {
        rt = newConnect(m_ip.c_str(), m_port);
    }
}

// 核心方法 CallMethod 负责将客户端的请求序列化并发送至服务端，并接受服务端响应对服务器响应进行反序列化
void Krpcchannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                             ::google::protobuf::RpcController* controller,
                             const ::google::protobuf::Message* request,        // 输入型参数
                             ::google::protobuf::Message* response,             // 输出型参数
                             ::google::protobuf::Closure* done) {
    if(m_clientfd == -1) {
        // 1. 在zookeeper中查询目标服务器的 IP : Port 
        const google::protobuf::ServiceDescriptor* sd = method -> service();
        service_name = sd -> name();
        method_name = method -> name();

        ZkClient zkCli;
        zkCli.Start();      // 完成对zookeeper服务器的连接
        
        std::string host_data = QueryServiceHost(&zkCli, service_name, method_name, m_idx);

        m_ip = host_data.substr(0, m_idx);
        std::cout << "ip :" << m_ip << std::endl;
        m_port = atoi(host_data.substr(m_idx + 1, host_data.size() - m_idx).c_str());
        std::cout << "port :" << m_port << std::endl;


        // 2. 尝试连接服务器
        bool rt = newConnect(m_ip.c_str(), m_port);
        if(!rt) {
            LOG(ERROR) << "Connect server error ";
            return;
        }else {
            LOG(INFO) << "Connect server success ";
        }

        // 3. request 序列化 (请求参数)
        std::string args_str;
        if(!request -> SerializeToString(&args_str)) {
            controller -> SetFailed("SerializeToString request fail");
            return;
        }
        
        // 4. 依照协议构造报文
        // 构造协议头部
        Krpc::RpcHeader KrpcHeader;
        KrpcHeader.set_service_name(service_name);
        KrpcHeader.set_method_name(method_name);
        KrpcHeader.set_args_size(args_str.size());       // 参数的二进制大小 即 body 大小  
        
        // 报头序列化
        std::string rpc_header_str;
        if(!KrpcHeader.SerializeToString(&rpc_header_str)){
            controller -> SetFailed("SerializeToString header error");
            return;
        }
        

        uint32_t header_size = rpc_header_str.size();
        uint32_t total_len = 4 + header_size + args_str.size();

        // 转网络字节序
        uint32_t net_total_len = htonl(total_len);
        uint32_t net_header_len = htonl(header_size);
        
        // 构造报文 
        std::string send_rpc_str;
        send_rpc_str.reserve(4 + 4 + header_size + args_str.size());
    
        send_rpc_str.append((char*)&net_total_len, 4);
        send_rpc_str.append((char*)&net_header_len, 4);
        send_rpc_str.append(rpc_header_str);
        send_rpc_str.append(args_str);     // 即request 

        // 发送报文
        if(send(m_clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0) == -1) {
            // 发送失败 关闭连接 
            close(m_clientfd);
            m_clientfd = -1;
            controller -> SetFailed("Send error");
            return;
        }


        // 接受服务端响应 [total_len] + [body]
        uint32_t response_len = 0;
        if(recv_exact(m_clientfd, (char*)&response_len, 4) != 4) {
            // 读取失败 关闭连接 
            close(m_clientfd);
            m_clientfd = -1;
            controller -> SetFailed("recv response_len error");
            return;
        }

        response_len = ntohl(response_len);

        // 接受body 
        std::vector<char> recv_buf(response_len);
        if(recv_exact(m_clientfd, recv_buf.data(), response_len) != response_len) {
            close(m_clientfd);
            m_clientfd = -1;
            controller -> SetFailed("recv body error");
            return;
        }

        // 对响应反序列化
        if(!response->ParseFromArray(recv_buf.data(), response_len)) { 
            close(m_clientfd);
            m_clientfd = -1;
            controller -> SetFailed("Parse response error");
            return;
        }

    }
}

// 循环读取 直至读取 size 字节
ssize_t Krpcchannel::recv_exact(int fd, char* buf, size_t size) {
    size_t total_read = 0;
    while(total_read < size) {
        ssize_t ret = recv(fd, buf + total_read, size - total_read, 0);
        if(ret == 0) {  // 对端关闭连接 
            return 0;
        }else if(ret == -1) {
            if(errno == EINTR)  continue;   // 阻塞式
            return -1;
        }
        
        total_read += ret;
    }

    
    return total_read;
}

// 从 zookeeper 获取服务器地址
// 调用ZKClient 的 GetData 获取 IP:PORT 
std::string Krpcchannel::QueryServiceHost(ZkClient *zkclient, std::string service_name, std::string method_name, int &idx) {
    std::string method_path = "/" + service_name + "/" + method_name;   // 规定的zookeeper路径
    std::cout << " method_path " << method_path << std::endl;  
    
    std::unique_lock<std::mutex> lock(g_data_mutex);
    std::string host_data_1 = zkclient -> GetData(method_path.c_str());
    
    lock.unlock();

    if (host_data_1 == "") {  // 如果未找到服务地址
        LOG(ERROR) << method_path + " is not exist!";  // 记录错误日志
        return " ";
    }

    idx = host_data_1.find(":");  // 查找IP和端口的分隔符
    if (idx == -1) {  // 如果分隔符不存在
        LOG(ERROR) << method_path + " address is invalid!";  // 记录错误日志
        return " ";
    }

    return host_data_1;  // 返回服务地址
}

bool Krpcchannel::newConnect(const char* ip, uint16_t port) {
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1) {
        char errtxt[512] = {0};
        std::cout << "socket error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;  // 打印错误信息
        LOG(ERROR) << "socket error:" << errtxt;  // 记录错误日志
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

        // 尝试连接服务器
    if (-1 == connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        close(clientfd);  // 连接失败，关闭socket
        char errtxt[512] = {0};
        std::cout << "connect error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;  // 打印错误信息
        LOG(ERROR) << "connect server error" << errtxt;  // 记录错误日志
        return false;
    }

    m_clientfd = clientfd;  // 保存socket文件描述符
    return true;
}




