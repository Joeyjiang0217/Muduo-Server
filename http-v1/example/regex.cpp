#include <iostream>
#include <string>
#include <regex>

int main()
{
    std::string str = "/number/123456";
    std::regex e("/number/(\\d+)"); // Regular expression to match "/number/" followed by digits
    std::smatch matches;

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