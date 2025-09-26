#include <iostream>
#include <string>
#include <functional>
#include <vector>

void print(const  std::string& msg, int num)
{
    std::cout << msg << " " << num << std::endl;
}

int main()
{
    using Task = std::function<void()>;
    std::vector<Task> array;
    array.push_back(std::bind(print, "Hello, World!", 1));
    array.push_back(std::bind(print, "Hello, Bind!", 2));
    array.push_back(std::bind(print, "Hello, aaaaaa", 3));
    array.push_back(std::bind(print, "Hello, bbbbbb", 4));
    for (auto& task : array) {
        task();
    }

    // print("Hello, World!", 1);
    // auto func = std::bind(print, "Hello, Bind!", std::placeholders::_1);
    // func(10);
    return 0;
}