#include<iostream>
#include<thread>
#include"Net_Message.h"
#include"Net_tsQueue.h"

//Đây sẽ là hàm chúng ta muốn thực thi trong luồng đó 
static bool s_Finish = false;

void DoWork()
{
    using namespace std::literals::chrono_literals;

    std::cout <<"Started at thread = "<<std::this_thread::get_id() <<'\n';

    while(!s_Finish)
    {
        std::cout<<"Working..\n";
        std::this_thread::sleep_for(1s);
    }
}

int main()
{
    /*Khi chúng ta viết hàm này ngay lập tức, khởi chạy luồng đó và thực hiện bất
    cứ thứ gì được viết bên trong
    */
    std::cout <<"Started at thread = "<<std::this_thread::get_id() <<'\n';
    std::thread employee(DoWork);

    std::cin.get();
    s_Finish=true;

    /*
        Chúng ta chờ đợi một thứ gì đó hoàn thành, hoặc chờ một luồng hoàn thành 
        công việc bằng cách. join
        "Này bạn có thể chờ trên luồng hiện tại cho đến khi luồng kia hoàn thành"

    */
    employee.join();
    std::cout<<"Finished!"<<'\n';
    std::cout <<"Started at thread = "<<std::this_thread::get_id() <<'\n';
    
    std::cin.get();
}