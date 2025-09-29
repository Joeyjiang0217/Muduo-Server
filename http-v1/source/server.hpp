#ifndef D9DBE1BB_CEE0_4A26_8BFF_B86806296AC4
#define D9DBE1BB_CEE0_4A26_8BFF_B86806296AC4

#include <vector>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <string>
#include <cstring>
#include <ctime>

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




#endif /* D9DBE1BB_CEE0_4A26_8BFF_B86806296AC4 */
