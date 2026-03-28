#ifndef _CONTROL_H_ 
#define _CONTROL_H_
#include <string>
#include <google/protobuf/service.h>

//RpcController 是抽象虚基类
class Krpccontroller : public google::protobuf::RpcController{
public:
    Krpccontroller();
    void Reset();                   // 重置状态机
    bool Failed() const;            // 是否失败
    std::string ErrorText() const;  // 获取错误信息
    void SetFailed(const std::string& reason);      // 设置错误信息
    
    // 未实现的虚函数
    void StartCancel(){}
    bool IsCanceled() const{return false;}
    void NotifyOnCancel(google::protobuf::Closure* callback){}

private:
    bool m_failed;
    std::string m_errText;
};

#endif // end of _CONTROL_H_ 

