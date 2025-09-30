#ifndef D9DBE1BB_CEE0_4A26_8BFF_B86806296AC4
#define D9DBE1BB_CEE0_4A26_8BFF_B86806296AC4

#include <vector>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <string>
#include <functional>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define INFO 0
#define DEBUG 1
#define ERROR 2
#define LOG_LEVEL DEBUG
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

#define BUFFER_DEFAULT_SIZE 1024
class Buffer
{
    private:
        std::vector<char> _buffer; // Internal buffer to store data
        uint64_t _reader_idx; // Read index
        uint64_t _writer_idx; // Write index
    public:
        Buffer()
            : _buffer(BUFFER_DEFAULT_SIZE), _reader_idx(0), _writer_idx(0)
        {}

        char* Begin()
        {
            return &(*_buffer.begin());
        }

        // get reader index
        char* WritePosition()
        {
            // The starting address of _buffer plus the write offset
            return Begin() + _writer_idx;
        }
        // get writer index
        char* ReadPosition()
        {
            // The starting address of _buffer plus the read offset
            return Begin() + _reader_idx;
        }

        // get behind idle space size -- after writer index
        uint64_t TailIdleSize()
        {
            return _buffer.size() - _writer_idx;
        }
        // get ahead idle space size -- before reader index
        uint64_t HeadIdleSize()
        {
            return _reader_idx;
        }

        // get readable size
        uint64_t ReadableSize()
        {
            return _writer_idx - _reader_idx;
        }

        // move reader index backward
        void MoveReadOffset(uint64_t len)
        {
            assert(len <= ReadableSize());
            _reader_idx += len;
        }
        // move writer index backward
        void MoveWriteOffset(uint64_t len)
        {
            // The backward movement must be less than the available free space 
            // behind the current position
            assert(len <= TailIdleSize() + HeadIdleSize());
            _writer_idx += len;
        }

        // ensure engough writable space 
        // (If there is sufficient space, move the data;
        // otherwise, expand the capacity.)
        void EnsureWriteableSpace(uint64_t len)
        {
            // If the available space behind the write index is enough, return directly
            if (TailIdleSize() >= len) 
            {
                return;
            }
            // If the total available space is enough, move the data to the front
            if (len <= TailIdleSize() + HeadIdleSize()) 
            {
                uint64_t readable_size = ReadableSize();
                // Move the readable data to the front
                std::copy(ReadPosition(), ReadPosition() + readable_size, Begin());
                _reader_idx = 0; // Reset read index to the beginning
                _writer_idx = readable_size; // Reset write index to the end of readable data
                return;
            }
            // Otherwise, expand the buffer
            _buffer.resize(_writer_idx + len);

        }

        // write data to buffer
        void Write(const void* data, uint64_t len)
        {
            EnsureWriteableSpace(len);
            std::copy((const char*)data, (const char*)data + len, WritePosition());
        }
        void WriteAndPush(const void* data, uint64_t len)
        {
            Write(data, len);
            MoveWriteOffset(len);
        }
        void WriteString(const std::string& data)
        {
            Write((const void*)data.c_str(), data.size());
        }
        void WriteStringAndPush(const std::string& data)
        {
            WriteString(data);
            MoveWriteOffset(data.size());
        }
        void WriteBuffer(Buffer& buf)
        {
            Write((void*)buf.ReadPosition(), buf.ReadableSize());
        }
        void WriteBufferAndPush(Buffer& buf)
        {
            WriteBuffer(buf);
            MoveWriteOffset(buf.ReadableSize());
        }

        // read data from buffer
        void Read(void* data, uint64_t len)
        {
            assert(len <= ReadableSize());
            std::copy(ReadPosition(), ReadPosition() + len, (char*)data);
        }
        void ReadAndPop(void* data, uint64_t len)
        {
            Read(data, len);
            MoveReadOffset(len);
        }
        std::string ReadAsString(uint64_t len)
        {
            assert(len <= ReadableSize());
            std::string result(ReadPosition(), len);
            return result;
        }
        std::string ReadAsStringAndPop(uint64_t len)
        {
            assert(len <= ReadableSize());
            std::string result = ReadAsString(len);
            MoveReadOffset(len);
            return result;
        }

        char* FindCRLF()
        {
            void* res = memchr(ReadPosition(), '\n', ReadableSize());
            return (char*)res;
        }

        //
        std::string GetLine()
        {
            char* pos = FindCRLF();
            if (pos == nullptr) {
                return "";
            }
            // include '\n'
            uint64_t len = pos - ReadPosition() + 1;
            return ReadAsStringAndPop(len);
        }

        std::string GetLineAndPop()
        {
            std::string result = GetLine();
            MoveReadOffset(result.size());
            return result;
        }

        // clear buffer
        void Clear()
        {
            _reader_idx = 0;
            _writer_idx = 0;
        }

};

#define MAX_LISTEN 1024
class Socket
{
    private:
        int _sockfd;
    public:
        Socket()
            : _sockfd(-1)
        {}
        Socket(int sockfd)
            : _sockfd(sockfd)
        {}
        ~Socket()
        {
            if (_sockfd != -1) {
                Close();
            }
        }

        int Fd()
        {
            return _sockfd;
        }
        // create socket
        bool Create()
        {
            // int socket(int domain, int type, int protocol);
            _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (_sockfd < 0) {
                ERROR_LOG("create socket error");
                return false;
            }
            return true;
        }
        // bind address information
        bool Bind(const std::string& ip, uint16_t port)
        {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(ip.c_str());
            socklen_t len = sizeof(addr);
            // int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
            int ret = bind(_sockfd, (struct sockaddr*)&addr, len);
            if (ret < 0) {
                ERROR_LOG("bind socket error");
                return false;
            }
            return true;
        }
        // listen
        bool Listen(int backlog = MAX_LISTEN)
        {
            // int listen(int sockfd, int backlog);
            int ret = listen(_sockfd, backlog);
            if (ret < 0) {
                ERROR_LOG("listen socket error");
                return false;
            }
            return true;
        }
        // initiate a connection to the server
        bool Connect(const std::string& ip, uint16_t port)
        {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(ip.c_str());
            socklen_t len = sizeof(addr);
            // int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
            int ret = connect(_sockfd, (struct sockaddr*)&addr, len);
            if (ret < 0) {
                ERROR_LOG("connect socket error");
                return false;
            }
            return true;
        }
        // obtain new connection
        int Accept()
        {
            // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
            int connfd = accept(_sockfd, nullptr, nullptr);
            if (connfd < 0) {
                ERROR_LOG("accept socket error");
                return -1;
            }
            return connfd;
        }
        // recv data
        ssize_t Recv(void* buf, size_t len, int flag = 0)
        {
            // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
            ssize_t ret = recv(_sockfd, buf, len, flag);
            if (ret <= 0) {
                // EAGAIN: The socket is set to non-blocking and there is no data available at the moment.
                // EINTR: The call was interrupted by a signal before any data was received.
                if (errno  == EAGAIN || errno == EINTR)
                {
                    return 0; // No data available now, try again later
                }
                return -1;
            }
            return ret;
        }

        ssize_t NonBlockRecv(void* buf, size_t len, int flag = 0)
        {
            return Recv(buf, len, MSG_DONTWAIT);
        }

        // send data
        ssize_t Send(const void* buf, size_t len, int flag = 0)
        {
            // ssize_t send(int sockfd, const void *buf, size_t len, int flags);
            ssize_t ret = send(_sockfd, buf, len, flag);
            if (ret <= 0) {
                ERROR_LOG("send socket error");
                return -1;
            }
            return ret; // return the number of bytes sent
        }

        ssize_t NonBlockSend(const void* buf, size_t len, int flag = 0)
        {
            return Send(buf, len, MSG_DONTWAIT);
        }
        // close socket
        void Close()
        {
            if (_sockfd != -1) {
                close(_sockfd);
                _sockfd = -1;
            }
        }
        // create a server-side connection
        bool CreateServer(uint16_t port, const std::string& ip = "0.0.0.0", bool block_flag = false)
        {
            // Create socket
            if (!Create()) {
                return false;
            }
            // Set Non-blocking
            if (block_flag) 
            {
                NonBlock();
            }

            // Bind address information
            if (!Bind(ip, port)) {
                return false;
            }
            // Listen
            if (!Listen()) {
                return false;
            }
            // Set reuse address
            ReuseAddr();
            return true;
        }
        // create a client-side connection
        bool CreateClient(uint16_t port, const std::string& ip, bool block_flag = false)
        {
            // Create socket
            if (!Create()) {
                return false;
            }
            // Set Non-blocking
            if (block_flag)
            {
                NonBlock();
            }
            // Connect to server
            if (!Connect(ip, port)) {
                return false;
            }
            return true;
        }
        // set socket options - enable address port reuse
        void ReuseAddr()
        {
            // int setsockopt(int sockfd, int level, int optname,
            //                const void *optval, socklen_t optlen);
            int val = 1;
            setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&val, sizeof(int));
            val = 1;
            setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void*)&val, sizeof(int));
        }
        // set socket blocking attribute - set to non-blocking
        void NonBlock()
        {
            // int flags = fcntl(sockfd, F_GETFL, 0);
            // fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
            int flags = fcntl(_sockfd, F_GETFL, 0);
            fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK);
        }

};

class Channel
{
    private:
        int _fd; // associated fd
        uint32_t _events; // interested events
        uint32_t _revents; // returned events
        using EventCallback = std::function<void()>;
        EventCallback _read_callback; // read event callback
        EventCallback _write_callback; // write event callback
        EventCallback _error_callback; // error event callback
        EventCallback _close_callback; // close event callback
        EventCallback _event_callback; // other event callback
    public:
        Channel(int fd)
            : _fd(fd), _events(0), _revents(0)
        {};

        int Fd()
        {
            return _fd;
        }

        void SetRevents(uint32_t revt)
        {
            _revents = revt;
        }

        void SetReadCallback(const EventCallback& cb)
        {
            _read_callback = cb;
        }
        void SetWriteCallback(const EventCallback& cb)
        {
            _write_callback = cb;
        }
        void SetErrorCallback(const EventCallback& cb)
        {
            _error_callback = cb;
        }
        void SetCloseCallback(const EventCallback& cb)
        {
            _close_callback = cb;
        }
        void SetEventCallback(const EventCallback& cb)
        {
            _event_callback = cb;
        }
        bool Readable()      // Has the readable file been monitored?
        {
            return _events & EPOLLIN;
        }
        bool Writable()      // Has the writable file been monitored?
        {
            return _events & EPOLLOUT;
        }
        void EnableRead()    // Enable monitoring for readable events
        {
            _events |= EPOLLIN;
        }
        void EnableWrite()   // Enable monitoring for writable events
        {
            _events |= EPOLLOUT;
        }
        void DisableRead()   // Disable monitoring for readable events
        {
            _events &= ~EPOLLIN;
        }
        void DisableWrite()  // Disable monitoring for writable events
        {
            _events &= ~EPOLLOUT;
        }
        void DisableAll()    // Disable monitoring for all events
        {
            _events = 0;
        }
        void Remove()        // Remove from epoll
        {
        }
        void HandleEvent() // handle events
        {
            if ( (_revents & EPOLLIN) || 
                 (_revents & EPOLLRDHUP) || 
                 (_revents & EPOLLPRI))
            {
                if (_read_callback) _read_callback();
                if (_event_callback) _event_callback();
            }

            //Process only one operation event at a time that may release a connection.
            if (_revents & EPOLLOUT)
            {
                if (_write_callback) _write_callback();
                if (_event_callback) _event_callback(); // Call after the event has been processed to refresh it.
            }
            else if (_revents & EPOLLERR)
            {
                if (_event_callback) _event_callback();
                // Once an error occurs, the connection is released,
                // so the event_callback must be called first.
                if (_error_callback) _error_callback();
            }
            else if (_revents & EPOLLHUP)
            {
                if (_event_callback) _event_callback();
                if (_close_callback) _close_callback();
            }


        }

};

class Connection
{

};



#endif /* D9DBE1BB_CEE0_4A26_8BFF_B86806296AC4 */
