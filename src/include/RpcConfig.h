#ifndef _CONFIG_H
#define _CONFIG_H

#include <unordered_map>
#include <string>

class Krpcconfig{
public:
    void LoadConfigFile(const char* config_file);       // 加载配置文件
    std::string Load(const std::string &key);           // input : key output: value
private:
    std::unordered_map<std::string, std::string> config_map;
    void Trim(std::string &read_buf);                   // 去掉字符串前后冗余的空格
};

#endif // end of  _CONFIG_H

