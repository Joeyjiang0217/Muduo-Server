#include <typeinfo>
#include <cassert>
#include <utility>
#include <iostream>
#include <string>
#include <unistd.h>
#include <any>
#include <memory>

class Any 
{
    private:
        class holder
        {
            public:
                virtual ~holder() {}
                virtual const std::type_info& type() = 0;
                virtual holder* clone() = 0;
        };
        template<class T>
        class placeholder : public holder
        {
            public:
                placeholder(const T& value) : _value(value) {}

                // Override type() to return the type of the stored value
                virtual const std::type_info& type() { return typeid(T); }
                // Override clone() to create a deep copy of the stored value
                virtual holder* clone() { return new placeholder(_value); }
            public:
                T _value;
        };
        holder* _content;
    public:
        Any() : _content(nullptr) {}
        template<class T>
        Any(const T& value) : _content(new placeholder<T>(value)) {}
        Any(const Any& other) : _content(other._content ? other._content->clone() : nullptr) {}
        ~Any() { delete _content; }

        Any& swap(Any& other)
        {
            std::swap(_content, other._content);
            return *this;
        }

        // Get the stored value
        template<class T>
        T* get()
        {
            // If _content is null or the type does not match, return nullptr
            assert(_content->type() == typeid(T));
            if (_content == nullptr)
            {
                return nullptr;
            }
            return &((placeholder<T>*)_content)->_value;
        }

        // Assignment operator
        template<class T>
        Any& operator=(const T& val)
        {
            // Use the copy-and-swap idiom for assignment
            Any(val).swap(*this);
            return *this;
        }
        Any& operator=(const Any& other)
        {
            // Use the copy-and-swap idiom for assignment
            Any(other).swap(*this);
            return *this;
        }
};

class Test
{
    public:
        Test() { std::cout << "Test()" << std::endl; }
        Test(const Test&) { std::cout << "Test(const Test&)" << std::endl; }
        ~Test() { std::cout << "~Test()" << std::endl; }
};

int main()
{
    std::any a;
    a = 10;
    int* pi = std::any_cast<int>(&a);
    std::cout << *pi << std::endl;

    a = std::string("hello");
    std::string* ps = std::any_cast<std::string>(&a);
    std::cout << *ps << std::endl;
    
    // {
    //     Any a;
    //     Test t;
    //     a = t;
    // }

    // a = 10;
    // int* pa = a.get<int>();
    // // double* pd = a.get<double>();
    // std::cout << *pa << std::endl;
    // a = std::string("nihao");
    // std::string* ps = a.get<std::string>();
    // std::cout << *ps << std::endl;

    // while (1)
    // {
    //     sleep(1);
    // }
    
    return 0;
}