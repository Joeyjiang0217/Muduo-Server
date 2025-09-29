#include "server.hpp"


int main()
{
    Buffer buf;
    

    for (int i = 0; i < 300; ++i) {
        std::string msg = "hello!!" + std::to_string(i) + "\n";
        buf.WriteStringAndPush(msg);
    }
    while (buf.ReadableSize() > 0) {
        std::string line = buf.GetLine();
        std::cout << line << std::endl;
    }
    // std::string tmp;
    // tmp = buf.ReadAsStringAndPop(buf.ReadableSize());
    // std::cout << tmp << std::endl;


    // buf.WriteStringAndPush(msg);

    // Buffer buf1;
    // buf1.WriteBufferAndPush(buf);


    
    // std::cout << tmp << std::endl;
    // std::cout << "ReadableSize: " << buf.ReadableSize() << std::endl;
    // std::cout << "ReadableSize: " << buf1.ReadableSize() << std::endl;
    return 0;
}