#ifndef __APP_H_ 
#define __APP_H_ 
#include "RpcConfig.h"
#include "RpcChannel.h"
#include "RpcController.h"  
#include <mutex>

// 负责初始化工作
class KrpcApplication{
public:
    static void Init(int argc, char** argv);
    static KrpcApplication& GetInstance();      // 单例模式
    static void deleteInstance();
    static Krpcconfig& GetConfig();
private:
    static Krpcconfig m_config;
    static KrpcApplication* m_application;
    static std::mutex m_mutex;
    KrpcApplication(){}
    ~KrpcApplication(){};

    KrpcApplication(const KrpcApplication&) = delete;
    KrpcApplication(KrpcApplication&&) = delete;
};


#endif 
