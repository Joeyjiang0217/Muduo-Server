#include "../source/server.hpp"

int main()
{
    Socket cli_sock;
    cli_sock.CreateClient(8081, "127.0.0.1");
    std::string str = "hello, world\n";
    cli_sock.Send(str.c_str(), str.size());
    char buf[1024] = {0};
    int ret = cli_sock.Recv(buf, 1023);
    DEBUG_LOG("recv %d bytes: %s", ret, buf);
    return 0;
}
