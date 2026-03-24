#include "include/Krpcapplication.h"
#include "include/Krpcconfig.h"
#include <cinttypes>
#include <cstdlib>
#include <mutex>
#include <unistd.h>
#include <iostream>

// 静态成员在类外初始化
Krpcconfig KrpcApplication::m_config;       
std::mutex KrpcApplication::m_mutex;
KrpcApplication* KrpcApplication::m_application = nullptr;

// 解析命令行参数 + 加载配置文件
void KrpcApplication::Init(int argc, char** argv){
    // 配置文件通过命令行参数读入
    if(argc < 2){
        std::cout << "Usage : command -i <配置文件路径> " << std::endl;
        exit(EXIT_FAILURE);
    }
    
    int c;
    std::string config_file;
    while((c = getopt(argc, argv, "i:")) != -1){
        switch(c){
            case 'i':
                config_file = optarg;
                break;
            case '?':
                std::cout << "Uasge : command -i <配置文件路径> " << std::endl;
                exit(EXIT_FAILURE);
                break;
            case ':':
                std::cout << "Uasge : command -i <配置文件路径> " << std::endl;
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }
    
    m_config.LoadConfigFile(config_file.c_str());
}

KrpcApplication& KrpcApplication::GetInstance(){
    //加锁保证线程安全
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_application == nullptr){
        m_application = new KrpcApplication();
        atexit(deleteInstance);  // 程序退出时销毁单例
    }
    return *m_application;
}

void KrpcApplication::deleteInstance(){
    if(m_application){
        delete m_application;
    }
}

Krpcconfig& KrpcApplication::GetConfig(){
    return m_config;
}




