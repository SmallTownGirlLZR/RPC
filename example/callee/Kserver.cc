#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <iostream>
#include <string>
#include "../user.pb.h"
#include "../../src/include/Krpcapplication.h"
#include "../../src/include/Krpcprovider.h"

class UserService : public Kuser::UserServiceRpc {  // 继承自 protobuf 生成的类
public:
    // 本地登录方法 
    bool Login(std::string name, std::string pwd) {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;  
        return true;  // 模拟登录成功
    }

    // 重写基类虚函数 用于RPC 
    // service -> CallMethod(method, nullptr, request, response, done); 
    void Login(::google::protobuf::RpcController* controller,    
                const ::Kuser::LoginRequest* request,
                ::Kuser::LoginResponse* response,
                ::google::protobuf::Closure* done){
        std::string name = request -> name();
        std::string pwd = request -> pwd();

        // 调用本地业务 
        bool login_result = Login(name, pwd);

        // 写入response
        Kuser::ResultCode* code = response -> mutable_result();
        code -> set_errcode(0);
        code -> set_errmsg("");
        response -> set_success(login_result);
        
        done -> Run();      // 服务端通过CallMethod 传递了闭包
    }
};

int main(int argc, char** argv) {
    // 调用框架的初始化操作，解析命令行参数并加载配置文件
    KrpcApplication::Init(argc, argv);

    // 创建一个 RPC 服务提供者对象
    KrpcProvider provider;

    // 将 UserService 对象发布到 RPC 节点上，使其可以被远程调用
    provider.NotifyService(new UserService());

    // 启动 RPC 服务节点，进入阻塞状态，等待远程的 RPC 调用请求
    provider.Run();

    return 0;
}




