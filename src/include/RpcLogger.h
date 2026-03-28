#ifndef _LOG_H 
#define _LOG_H
#include <glog/logging.h>
#include <string>

// RAII 设计  
class KrpcLogger{
public:
    //  正确且唯一的实例化方式
    //Krpciogger logger("my_rpc_server");
    explicit KrpcLogger(const char* argv0){
        google::InitGoogleLogging(argv0);
        fLB::FLAGS_colorlogtostderr = true; // 彩色日志
        fLB::FLAGS_logtostderr = true;      // 默认输出标准错误,而不是存储到磁盘上 /tmp
    }

    ~KrpcLogger(){
        google::ShutdownGoogleLogging();
    }

    static void Info(const std::string& message){
        LOG(INFO) << message;
    }

    static void Warning(const std::string &message){
        LOG(WARNING)<<message;
    }
    static void ERROR(const std::string &message){
        LOG(ERROR)<<message;
    }
    static void Fatal(const std::string& message) {
        LOG(FATAL) << message;
    }


private:
    KrpcLogger(const KrpcLogger&) = delete;
    KrpcLogger& operator= (const KrpcLogger&) = delete;
};


#endif // !LOG 

