#include "CYCoroutine/CYCoroutine.hpp"
#include <iostream>

#include <thread>

int main(int argc, const char * argv[]) {
    std::cout << "Main threadId=" << std::this_thread::get_id() << std::endl;

    auto result = CYInlineCoro()->Submit([]()-> int {
        std::cout << "CYInlineCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
        return 5;
        });

   result.Get();

   auto result2 = CYThreadPoolCoro()->Submit([]()-> int {
       std::cout << "CYThreadPoolCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
       return 5;
       });

   result2.Get();

   auto result3 = CYBackgroundCoro()->Submit([]()-> int {
       std::cout << "CYBackgroundCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
       return 5;
       });

   result3.Get();

   auto result4 = CYThreadCoro()->Submit([]()-> int {
       std::cout << "CYThreadCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
       return 5;
       });

   result4.Get();



   CYCoroFree();


   return 0;}
