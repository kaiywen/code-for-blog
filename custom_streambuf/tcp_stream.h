#include <iostream>

namespace tcpstream {

class TcpStreamBuf : public std::streambuf {
public:
    TcpStreamBuf(int socket, size_t buf_size);
    ~TcpStreamBuf();

    int underflow();
    int overflow(int c);
    int sync();

private:
    const size_t buf_size_;
    int socket_;
    char* pbuf_;
    char* gbuf_;
};

class BasicTcpStream : public std::iostream {
public:
    BasicTcpStream(int socket, size_t buf_size);
    ~BasicTcpStream();

private:
    int socket_;
    const size_t buf_size_;
};

}  // tcpstream

