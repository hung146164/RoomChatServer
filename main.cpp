#include<iostream>
#include<asio.hpp>
#include<asio/ts/buffer.hpp>
#include<asio/ts/internet.hpp>
#include<chrono>
#include<thread>

std::vector<char> vBuffer(20*1024);

void GrabSomeData(asio::ip::tcp::socket& socket)
{
    // Before when we synchronization we call readsum()
    // Now asynchronization
    socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size())
    ,[&](std::error_code ec, std::size_t length)
    {
        if(!ec)
        {
            std::cout<<"\n\nRead "<<length<<" Bytes\n\n";

            std::cout.write(vBuffer.data(), length);
            
            GrabSomeData(socket);
        }
        else if (ec == asio::error::eof)
        {
            std::cout << "\nConnection closed by server\n";
        }
        else
        {
            std::cout << "Error: " << ec.message() << "\n";
        }
    });
}

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

    //give some fake tasks to asio so the context doesnt finish
    auto work = asio::make_work_guard(context);

    //start Context 
    std::thread thrContext = std::thread([&](){context.run();});

    //Get the address of somewhere we wish connect to
    /*
    - An Endpoint is a Combination of an IP address and a Port
    - asio::ip::make_address("93.184.216.34", ec): Converts an IP string 
    into binary format that the computer understands 
    (Eg: 93.184.216.34 is example.com)
    - 80: The default port of HTTP protocol 
    - This step only for declaration; it does not establish a connection yet.
    */
    asio::ip::tcp::resolver resolver(context);
auto endpoints = resolver.resolve("httpbin.org", "80");
   
    //Create a socket, the context will deliver the implement
    /*
       - The socket is initialized with the context mentioned above.
       - At this stage, the socket has been created but its state is closed. 
       It is like buying a new phone but not making a call yet."
    */
    asio::ip::tcp::socket socket(context);

    //Tell Socket to try and connect 
    std::cout << "Connecting..." << std::endl;
    //socket.connect(endpoint, ec);

    asio::async_connect(socket, endpoints,
    [&](std::error_code ec, asio::ip::tcp::endpoint)
    {
        /*
        - Before performing any operation, we must verify that the connections is 
        established 
        - If the connection fails or the socket is closed, attempting to write data will 
        cause the program to crash or lead to serious errors
        */
        if (!ec)
        {
            std::cout << "Connected\n";

            GrabSomeData(socket);

            /*
            - I want to fetch the index.html file using the HTTP/1.1 protocol.
            - Specifies the target server (since one IP address can host multiple websites).
            - Tells the server to close the connection once the data transfer is complete.
            */
            std::string sRequest = 
                "GET / HTTP/1.1\r\n"
                "Host: httpbin.org\r\n"
                "Connection: close\r\n\r\n";

            /*
            - asio::buffer(): The network doesn't understand strings; it only processes byte arrays.
            - It creates a "buffer view" (a wrapper around a pointer and its metadata).
            - This view stores the starting memory address of 'sRequest' and its size.
            - Note: The view object itself can be reassigned to a different memory block, 
            whereas a reference cannot.

            - sRequest.data(): Returns a pointer to the string's internal storage.
            (const char* ptr = sRequest.data(); // Points to the first byte)
            - sRequest.size(): The number of bytes to be sent.

            - asio::write(socket, buffer): 
            Equates to: while (not all data sent) { write_some(...) }
            
            - Why choose write_some over write?
            asio::write is a composite operation that blocks until the entire buffer is sent.
            write_some allows for partial sends: send a portion -> return control to the program 
            -> perform other tasks -> send the rest later. This prevents the application from 
            stalling on large data transfers.
            */
            asio::async_write(socket, asio::buffer(sRequest),
            [&](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    std::cout << "Request sent (" << length << " bytes)\n";
                }
                else
                {
                    std::cout << "Write Error: " << ec.message() << "\n";
                }
            });
        }
        else
        {
            std::cout << "Connect failed: " << ec.message() << "\n";
        } 
    });
    thrContext.join();
       
    return 0; 
}