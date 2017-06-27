#include <unistd.h>
#include <assert.h>
#include <stddef.h>
#include <sys/socket.h>

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
        int ret = send(socket_, pbase()+sent, total-sent, 0);
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
    if (-1 == sync()) {
        return traits_type::eof();
    }
    else {
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            sputc(traits_type::to_char_type(c));
        }

        // return eq_int_type(c, eof()) ? eof():c;
        return traits_type::not_eof(c);
    }
}

int TcpStreamBuf::underflow() {
    int ret = recv(socket_, eback(), buf_size_, 0);
    if (ret > 0) {
        setg(eback(), eback(), eback() + ret);
        return traits_type::to_int_type(*gptr());
    } else {
        return traits_type::eof();
    }
    return 1;
}

BasicTcpStream::BasicTcpStream(int socket, size_t buf_size):
    std::iostream(new TcpStreamBuf(socket, buf_size)), 
    socket_(socket), buf_size_(buf_size) {
}

BasicTcpStream::~BasicTcpStream() {
    delete rdbuf();
}

}  // tcpstream

