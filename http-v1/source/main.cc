#include "server.hpp"

#define INFO 0
#define DEBUG 1
#define ERROR 2
#define LOG_LEVEL INFO
#define LOG(level, format, ...) do{\
    if (level < LOG_LEVEL) break;\
    time_t t = time(NULL);\
    struct tm *ltm = localtime(&t);\
    char tmp[32] = {0};\
    strftime(tmp, 31, "%H:%M:%S", ltm);\
    fprintf(stdout, "[%s %s:%d] " format "\n", tmp, __FILE__, __LINE__, ##__VA_ARGS__);\
}while(0)
#define INFO_LOG(format, ...) LOG(INFO, format, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) LOG(DEBUG, format, ##__VA_ARGS__)
#define ERROR_LOG(format, ...) LOG(ERROR, format, ##__VA_ARGS__)

int main()
{
    Buffer buf;
    

    for (int i = 0; i < 300; ++i) {
        std::string msg = "hello!!" + std::to_string(i) + "\n";
        buf.WriteStringAndPush(msg);
    }
    while (buf.ReadableSize() > 0) {
        std::string line = buf.GetLine();
        // std::cout << line << std::endl;
        INFO_LOG("%s", line.c_str());
    }
    INFO_LOG("Hello, World!");
    INFO_LOG("Hello, World!");
    INFO_LOG("Hello, World!");
    INFO_LOG("Hello, World!");
    INFO_LOG("Hello, World!");
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