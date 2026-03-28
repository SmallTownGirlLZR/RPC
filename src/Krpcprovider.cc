#include "include/Krpcapplication.h"
//#include "Krpcheader.pb.h"
#include "Krpcheader.pb.h"
#include "include/Krpcprovider.h"
#include <cstdint>
#include <cstring>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <google/protobuf/stubs/port.h>
#include <iostream>
#include <memory>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <string>
#include <sys/types.h>
#include "include/KrpcLogger.h"

/* 
 *注册服务
 *实际上是想service_map<string, ServiceInfo> 中插入数据
 *先构造serviceInfo 并获取 服务名 
 *再进行插入
 * */
void KrpcProvider::NotifyService(google::protobuf::Service* service) {
    ServiceInfo service_info;
    service_info.service = service;

    const google::protobuf::ServiceDescriptor* psd = service -> GetDescriptor();
    std::string service_name = psd -> name();
    int method_count = psd -> method_count();

    std::cout << "service_name : " << service_name << std::endl;
    for(int i = 0; i < method_count; i++){
        const google::protobuf::MethodDescriptor* pmd = psd -> method(i);
        std::string  method_name = pmd -> name();
        std::cout << "method_name : " << method_name << std::endl;
        service_info.method_map[method_name] = pmd;
    }

    service_map[service_name] = service_info;
}

// callBack 
// ? 有问题  
void KrpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn){
    if(!conn -> connected()){
        conn -> shutdown();
    } 
}

// 初始化TcpServer 并绑定回调方法
void KrpcProvider::Run(){
    // 获取 IP & Port 配置          
    std::string ip = KrpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    int port = atoi(KrpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    
    LOG(INFO) << "ip : %s" << ip;
    LOG(INFO) << "port : %d " << port;


    // 创建地址对象
    muduo::net::InetAddress address(ip, port);

    // 创建TcpServer
    std::shared_ptr<muduo::net::TcpServer> server = std::make_shared<muduo::net::TcpServer>(&event_loop, address, "KrpcProvider");
 
    // 绑定回调
    server -> setConnectionCallback(
            std::bind(&KrpcProvider::OnConnection, this, std::placeholders::_1) 
    );
    server -> setMessageCallback(
            std::bind(&KrpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)  
    );

    server -> setThreadNum(4);

    // 将当前RPC节点上要发布的服务全部注册到ZooKeeper上，让RPC客户端可以在ZooKeeper上发现服务
    ZkClient zkclient;
    zkclient.Start();  // 连接ZooKeeper服务器
    // service_name为永久节点，method_name为临时节点
    for (auto &sp : service_map) {
        // service_name 在ZooKeeper中的目录是"/"+service_name
        std::string service_path = "/" + sp.first;
        zkclient.Create(service_path.c_str(), nullptr, 0);  // 创建服务节点
        for (auto &mp : sp.second.method_map) {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);  // 将IP和端口信息存入节点数据
            // ZOO_EPHEMERAL表示这个节点是临时节点，在客户端断开连接后，ZooKeeper会自动删除这个节点
            zkclient.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // RPC服务端准备启动，打印信息
    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

    // 启动网络服务
    server->start();
    event_loop.loop();  // 进入事件循环
}

// 有数据可读时
void KrpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp){
    std::cout << "OnMessage " << std::endl;
    // 解决Tcp粘包 和 拆包 while循环就是解决粘包处理多个包
    // [Total Len 4 byte][Header Len 4 Bytes][--Header--][Body]
    // Header 存储的是序列化后的Krpcheader,包含service_name,method_name,args_size
    // Body 是 method 规定的request
    while(buffer -> readableBytes() >= 4){
        // step 1 : 预读千 4 个字节 获得总长度
        uint32_t total_len = 0;
        std::memcpy(&total_len, buffer -> peek(), 4);
        total_len = ntohl(total_len);
        
        // 数据不完整 (发生了拆包)
        if(total_len + 4 > buffer -> readableBytes()){
            break;
        }

        //----- 数据包完整 开始处理 --------
        buffer -> retrieve(4);
        
        // step 2 : 读取 4 个字节 获取Header 的长度
        uint32_t header_len = 0;
        const char* data_ptr = buffer -> peek();
        std::memcpy(&header_len, data_ptr,4);
        buffer -> retrieve(4);
        
        // step 3 : 读取Header 的数据 
        std::string rpc_header_str(buffer -> peek(), header_len);
        Krpc::RpcHeader krpcHeader;
        buffer -> retrieve(header_len);
 
        // step 4 : 读取Body 数据 (方法的参数args)
        uint32_t args_size = total_len - 4 - header_len;
        std::string args_str(buffer -> peek(), args_size);

        buffer -> retrieve(args_size);
        
        // 反序列化 string --> 结构体 
        if(!krpcHeader.ParseFromString(rpc_header_str)) {
            std::cout << "Header parse error" << std::endl;
            return;
        }
        
        std::string service_name = krpcHeader.service_name();
        std::string method_name = krpcHeader.method_name(); 
        
        // < service_name string, service_info ServiceInfo>
        auto it = service_map.find(service_name);
        if(it == service_map.end()){
            std::cout << service_name << "is not exist !" << std::endl;
            return;
        }
        
        auto mit = it -> second.method_map.find(method_name);
        if(mit == it -> second.method_map.end()){
            std::cout << service_name << " . " << method_name << "is not exist" << std::endl;
            return; 
        }

        google::protobuf::Service* service = it -> second.service;
        const google::protobuf::MethodDescriptor* method = mit -> second;
        
        // new 的返回值是method 函数规定的参数 
        google::protobuf::Message* request = service -> GetRequestPrototype(method).New();
        if(!request -> ParseFromString(args_str)){
            std::cout << "request parse error " << std::endl;
            return;
        }
        // new 的返回值是method函数规定的返回值
        // 所以在写protobuf的service时，返回值必须也是protobuf生成的 否则无法被Messgae接收
        // 这样是合法的
        //rpc GetInfo(MyRequest) returns (MyResponse);
        // 这样是不合法的，protoc 编译器直接报错，根本不会生成 C++ 代码
        //rpc GetInfo(int32) returns (string);
        google::protobuf::Message *response = service -> GetResponsePrototype(method).New();
        
        // 提前绑定好"如何发送响应"
        google::protobuf::Closure* done = google::protobuf::NewCallback< 
            KrpcProvider,                           // 类名 表明是此类的成员函数
            const muduo::net::TcpConnectionPtr&,    // 函数的第一个参数类型
            google::protobuf::Message*              // 函数的第二个参数类型
        >(  // 实际传入的参数部分 
            this,                           // 执行该函数的对象
            &KrpcProvider::SendRpcResponse, // 函数指针 
            conn,                             // 参数 1
            response                          // 参数 2 
        );
        
        // 下发任务给业务层
        // 在method的函数中 调用done -> run()
        // 这里使用了多态 service将向下转型，
        // 起到路由作用 如果调用mehthod 则将done这个闭包传递给mehthod
        service -> CallMethod(method, nullptr, request, response, done); 
    }

}
// 发送响应给客户端
// 响应没有Header 吗 ？？？
void KrpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response){
    std::string response_str;
    if(response -> SerializeToString(&response_str)){   // 序列化
        // [4 bytes total_len]  + [Response Data]
        uint32_t len = response_str.size();
        uint32_t net_len = htonl(len);

        std::string send_buf;
        send_buf.resize(4 + len);

        std::memcpy(&send_buf[0], &net_len, 4);
        std::memcpy(&send_buf[4], response_str.data(), len);
    }else{
        std::cout << "service response err" << std::endl;
    }
}


KrpcProvider::~KrpcProvider() {
    std::cout << "~KrpcProvider " << std::endl;
    event_loop.quit();
}
