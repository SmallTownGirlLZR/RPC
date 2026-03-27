#include <atomic>
#include <iostream>
#include "../user.pb.h"
#include <thread>
#include "../../src/include/Krpcapplication.h"
#include "../../src/include/Krpcchannel.h"

#include "../../src/include/KrpcLogger.h"
// 发送RPC 请求的函数
void send_request(int thread_id, std::atomic<int>& success_count, std::atomic<int>& fail_count, int requests_per_thread) {
    Kuser::UserServiceRpc_Stub stub(new Krpcchannel(false));
    Kuser::LoginRequest request;

    request.set_name("LinZR");
    request.set_pwd("123456");

    // RPC 响应
    Kuser::LoginResponse response;
    Krpccontroller controller;

    for(int i = 0; i < requests_per_thread; i++) {
        stub.Login(&controller, &request, &response, nullptr);

        if(controller.Failed()){    // 调用失败
            std::cout << controller.ErrorText() << std::endl;
            fail_count++;
        }else{
            if (int{} == response.result().errcode()) {  // 检查响应中的错误码
                std::cout << "rpc login response success:" << response.success() << std::endl;  // 打印成功信息
                success_count++;  // 成功计数加 1
            } else {  // 如果响应中有错误
                std::cout << "rpc login response error : " << response.result().errmsg() << std::endl;  // 打印错误信息
                fail_count++;  // 失败计数加 1
            }
        }

    } 
}

int main(int argc, char* argv[]){
   // 初始化 RPC 框架，解析命令行参数并加载配置文件
    KrpcApplication::Init(argc, argv);

    // 创建日志对象
    KrpcLogger logger("MyRPC");

const int thread_count = 100;      // 线程数改为 100
const int requests_per_thread = 5000; // 每个线程发 5000 次请求

    std::vector<std::thread> threads;  // 存储线程对象的容器
    std::atomic<int> success_count(0); // 成功请求的计数器
    std::atomic<int> fail_count(0);    // 失败请求的计数器

    auto start_time = std::chrono::high_resolution_clock::now();  // 记录测试开始时间

    // 启动多线程进行并发测试
    for (int i = 0; i < thread_count; i++) {
        threads.emplace_back([argc, argv, i, &success_count, &fail_count, requests_per_thread]() {  
                send_request(i, success_count, fail_count,requests_per_thread);  // 每个线程发送指定数量的请求
        });
    }

    // 等待所有线程执行完毕
    for (auto &t : threads) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();  // 记录测试结束时间
    std::chrono::duration<double> elapsed = end_time - start_time;  // 计算测试耗时

    // 输出统计结果
    LOG(INFO) << "Total requests: " << thread_count * requests_per_thread;  // 总请求数
    LOG(INFO) << "Success count: " << success_count;  // 成功请求数
    LOG(INFO) << "Fail count: " << fail_count;  // 失败请求数
    LOG(INFO) << "Elapsed time: " << elapsed.count() << " seconds";  // 测试耗时
    LOG(INFO) << "QPS: " << (thread_count * requests_per_thread) / elapsed.count();  // 计算 QPS（每秒请求数）

    return 0; 
}










