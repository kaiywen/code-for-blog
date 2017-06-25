#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "tcp_stream.h"

static const int BUF_SIZE = 5;

// just for test 
int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    struct sockaddr_in in_addr;
    memset(&in_addr, 0, sizeof(in_addr));
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    in_addr.sin_port = htons(10010);

    int ret = connect(sockfd, (struct sockaddr*) &in_addr, sizeof(in_addr));
    if (0 != ret && ((errno != EINPROGRESS) && (errno != EAGAIN))) {
        std::cout << "cannot connect: " << ret << std::endl;
        return 0;
    }

    int nodelay = 1;
    if(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay)) < 0) {
        std::cout << "set nodelay error" << std::endl;
        close(sockfd);
        return 0;
    }

    tcpstream::BasicTcpStream tcpInOut(sockfd, BUF_SIZE);
    char line[64] = { 0 };

    if(tcpInOut.getline(line, 64).good()) {
        std::cout << line << std::endl;
    }
    else {
        std::cout << "receive error: " << line << std::endl;
    }

    char c;
    while(tcpInOut >> c) {
        std::cout << c << std::endl;
        tcpInOut << std::toupper(c) << std::endl; 
        if(c == 'q') {
            close(sockfd);
            break;
        }
    }
}
