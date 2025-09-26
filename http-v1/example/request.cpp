#include <iostream>
#include <string>
#include <regex>

int main()
{
    // HTTP request example: GET /tcpserver/login?user=joe&password=123456 HTTP/1.1\r\n
    std::string str = "GET /tcpserver/login?user=joe&password=123456 HTTP/1.1\r\n";
    std::smatch matches;
    // 1. Match the request line
    // GET HEAD POST PUT DELETE
    std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?"); // Regular expression to match HTTP request line
    // GET|HEAD|POST|PUT|DELETE: Match the HTTP method
    // ([^?]*): Match the URL path (everything except '?')
    // \\?(.*): Match the query string (everything after '?')
    // (HTTP/1\\.[01]): Match the HTTP version
    // (?:\n|\r\n)?: Match the line ending (non-capturing group)

    bool ret = std::regex_match(str, matches, e);
    if (ret == false)
    {
        std::cerr << "Invalid request format" << std::endl;
        return -1;
    }

    for (auto& s : matches)
    {
        std::cout << s << std::endl;
    }
    return 0;
}