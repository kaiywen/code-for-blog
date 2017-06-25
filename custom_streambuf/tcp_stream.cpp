#include <unistd.h>
#include <assert.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include "tcp_stream.h"

namespace tcpstream {

TcpStreamBuf::TcpStreamBuf(int socket, size_t buf_size) :
    buf_size_(buf_size), socket_(socket) {
    assert(buf_size_ > 0);
    pbuf_ = new char[buf_size_]; 
    gbuf_ = new char[buf_size_];

    setp(pbuf_, pbuf_ + buf_size_); // set the pointers for output buf
    setg(gbuf_, gbuf_, gbuf_);      // set the pointers for input buf
}

TcpStreamBuf::~TcpStreamBuf() {
    if(pbuf_ != nullptr) {
        delete pbuf_;
        pbuf_ = nullptr;
    }

    if(gbuf_ != nullptr) {
        delete gbuf_;
        gbuf_ = nullptr;
    }
}

// flush the data to the socket
int TcpStreamBuf::sync() {
    int sent = 0;
    int total = pptr() - pbase();
    while (sent < total) {
        int ret = send(socket_, pbuf_, buf_size_, 0);
        if (ret > 0) sent += ret;
        else {
            return -1;
        }
    }
    setp(pbase(), pbase() + buf_size_);
    pbump(0);  // reset the pptr

    return 0;
}

int TcpStreamBuf::overflow(int c) {
}

int TcpStreamBuf::underflow() {
    int ret = recv(socket_, gbuf_, buf_size_, 0);

}

}  // tcpstream

