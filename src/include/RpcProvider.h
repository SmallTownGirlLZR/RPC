#ifndef PROVIDER
#define PROVIDER
#include "google/protobuf/service.h"
#include "ZookeeperUtil.h"
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <google/protobuf/descriptor.h>
#include <functional>
#include <string>
#include <unordered_map>

// 服务端代码
class KrpcProvider{
public:
    //核心作用：服务注册。将本地编写的 RPC 服务对象发布到 RPC 框架中。
    void NotifyService(google::protobuf::Service* service);
    ~KrpcProvider();
    void Run();
private:
    struct ServiceInfo{
        google::protobuf::Service* service;     // 所有service的父类 Service，在这里指向一个具体的服务（类） 
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> method_map;  //MethodDescriptor 描述函数的metadata
    };
    muduo::net::EventLoop event_loop;
    std::unordered_map<std::string, ServiceInfo> service_map;   // 保存服务对象和Rpc方法
    
    void OnConnection(const muduo::net::TcpConnectionPtr& conn);    // muduo TcpServer回调函数
    void OnMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp); // 回调函数 
    void SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response);    //Message* 是基类指针                                                                
};

#endif // end of PROVIDER
