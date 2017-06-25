#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "tcp_stream.h"

static const int BUF_SIZE = 5;

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    struct sockaddr_in in_addr;
    memset(&in_addr, 0, sizeof(in_addr));
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    in_addr.sin_port = htons(10010);

    int flags = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &flags, sizeof(flags)) < 0) {
        std::cout << "set reused flag error" << std::endl;
        return 0;
    }

    if (bind(sockfd, (struct sockaddr*) &in_addr, sizeof(in_addr)) < 0) {
        std::cout << "bind error" << std::endl;
        close(sockfd);
        return 0;
    }

    if(listen(sockfd, 1024) < 0) {
        std::cout << "listen error" << std::endl;
        close(sockfd);
        return 0;
    }

    for (;;) {
        int clientfd = -1;
        struct sockaddr_in addr;
        socklen_t socklen = sizeof(addr);

        clientfd = accept(sockfd, (struct sockaddr*) &addr, &socklen);

        if (clientfd >= 0) {
            tcpstream::BasicTcpStream tcpInOut(clientfd, BUF_SIZE);
            tcpInOut << "Hello World" << std::endl;

            for(char c = 'j'; c <= 'q'; c++) {
                tcpInOut << c << std::endl;
            }

            int rc;
            while(tcpInOut >> rc) {
                std::cout << char(rc) << std::endl;
            }
        }
    }

    return 1;
}
