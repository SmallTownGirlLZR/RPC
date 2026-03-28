#include "include/RpcController.h" 

Krpccontroller::Krpccontroller(){
    m_failed = false;
    m_errText = "";
}

void Krpccontroller::Reset(){
    m_failed = false;
    m_errText = "";
}

bool Krpccontroller::Failed() const{
    return m_failed;
}

std::string Krpccontroller::ErrorText() const{
    return m_errText;
}

void Krpccontroller::SetFailed(const std::string& reason){
    m_failed = true;
    m_errText = reason;
}
