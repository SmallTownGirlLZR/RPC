#ifndef _Krpcchannel_h_
#define _Krpcchannel_h_
#include <cstdint>
#include <google/protobuf/service.h>
#include <queue>
#include <sys/types.h>
#include "zookeeperutil.h"

class Krpcchannel : public google::protobuf::RpcChannel {
public:
    Krpcchannel(bool connectNow);
    virtual ~Krpcchannel() {
        if(m_clientfd >= 0) {
            close(m_clientfd);
        }
    }
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done) override;
private:
    int m_clientfd;     // 客户端套接字
    std::string service_name;
    std::string method_name;
    std::string m_ip;
    uint16_t m_port;
    int m_idx;  // index 划分IP和port

    bool newConnect(const char* ip, uint16_t port);
    std::string QueryServiceHost(ZkClient *zkclient, std::string service_name, std::string method_name, int &idx);
       // 新增：确保读取指定长度的数据，解决TCP拆包
    ssize_t recv_exact(int fd, char* buf, size_t size); 
};


#endif 
