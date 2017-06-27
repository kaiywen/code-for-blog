#ifndef __BASIC_TCP_STREAM__
#define __BASIC_TCP_STREAM__

#include <iostream>

namespace tcpstream {

// a simple stream buf for tcp socket
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

/////////////////////////////

// a simple stream implementation
class BasicTcpStream : public std::iostream {
public:
    BasicTcpStream(int socket, size_t buf_size);
    ~BasicTcpStream();

private:
    int socket_;
    const size_t buf_size_;
};

}  // tcpstream

#endif // __BASIC_TCP_STREAM__
