#include<iostream>
#include<asio.hpp>
#include<asio/ts/buffer.hpp>
#include<asio/ts/internet.hpp>

int main()
{
    std::cout << "Starting program..." << std::endl;
    /*
        Why we have it?
        In Network programming, errors occur frequently (network loss, wrong addresses, 
        timeout,...)
        Instead of using try-catch(Exceptions) which can slow down program, Asio uses
        error_code objects to store error infomation. If an operation fails, 
        the 'ec' variable will be assigned an error value, and if succeeds, it returns 0;
    */
    asio::error_code ec;

    //Create a "context" - essentially the platform specific interface
    /*
        This is the 'Heart' of Asio application
        Objective: It atcs as a bridge between your program and OS(linux,window,...)
        All I/O operations(sending data, receiving data, connecting) must pass through 
        this entity to manage hardware/network interactions.
    */
    asio::io_context context;

    //Get the address of somewhere we wish connect to
    /*
    - An Endpoint is a Combination of an IP address and a Port
    - asio::ip::make_address("93.184.216.34", ec): Converts an IP string 
    into binary format that the computer understands 
    (Eg: 93.184.216.34 is example.com)
    - 80: The default port of HTTP protocol 
    - This step only for declaration; it does not establish a connection yet.
    */
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1",ec), 4545);
   
    //Create a socket, the context will deliver the implement
    /*
       - The socket is initialized with the context mentioned above.
       - At this stage, the socket has been created but its state is closed. 
       It is like buying a new phone but not making a call yet."
    */
    asio::ip::tcp::socket socket(context);

    //Tell Socket to try and connect 
    socket.connect(endpoint, ec);
    std::cout << "Attempting to connect..." << std::endl;

    if(!ec)
    {
        std::cout<<"Connected"<<std::endl;
    }
    else{
        std::cout<<"Failed to connect to address: \n" << ec.message() <<std::endl;
    }

    return 0;
}