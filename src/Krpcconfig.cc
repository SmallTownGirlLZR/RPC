#include "include/Krpcconfig.h"
#include <cstdlib>
#include <memory>
#include <cstdio>
#include <stdlib.h>
#include <string>


void Krpcconfig::LoadConfigFile(const char* config_file){
    // 定义删除器为fclose
    // 由于FILE不是new出来的 无法调用delete 
    // 所以必须自定义删除器
    std::unique_ptr<FILE, decltype(&fclose)> pf(
        fopen(config_file, "r"),  // 打开配置文件
        &fclose  // 文件关闭函数
    );

    if(pf == nullptr){
        exit(EXIT_FAILURE);
    }
    char buf[1024];
    while(fgets(buf,sizeof(buf) - 1, pf.get()) != nullptr) {
        std::string read_buf(buf);
        Trim(read_buf);  // 去除字符串前后的空格
        // 遇到注释和空行直接跳过
        if(read_buf[0] = '#' || read_buf.empty())
            continue;

        int index = read_buf.find("=");
        if(index == std::string::npos) continue;    // 不合法的语句
        // 获取key
        std::string key = read_buf.substr(0, index);
        Trim(key);
        
        // 获取value
        int endindex = read_buf.find("\n");
        std::string value = read_buf.substr(index + 1, endindex - index - 1);
        Trim(value);

        config_map.insert({key, value}); 
    }
}
/*
 * 低性能 调用了一次count一次[]运算符 
std::string Krpcconfig::Load(const std::string& key){
    if(config_map.count(key)){
        return config_map[key];
    }else{
        return "";
    }
}
*/

std::string Krpcconfig::Load(const std::string& key){
    auto it = config_map.find(key);
    if(it == config_map.end()){
        return "";
    }else{
        return it -> second;
    }
}


void Krpcconfig::Trim(std::string& read_buf){
    // 去除前导0
    int index = read_buf.find_first_not_of(' ');
    if(index != std::string::npos){
        read_buf = read_buf.substr(index, read_buf.size() - index);
    }
    
    index = read_buf.find_last_not_of(' ');
    if(index != std::string::npos){
        read_buf = read_buf.substr(0,index + 1);
    }

}





