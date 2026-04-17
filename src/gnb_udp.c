/*
   Copyright (C) gnbdev

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#endif

#ifdef _WIN32
#define _POSIX
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "gnb_udp.h"

/*
 * 调节 UDP socket 的性能参数:
 *   - 扩大 SO_RCVBUF/SO_SNDBUF，避免在突发流量下内核丢包
 *   - 非 Windows 平台设置 O_NONBLOCK，让上层可以做 drain 式批量读
 *
 * 注意: 需要内核允许这么大的缓冲区,参考:
 *   sysctl -w net.core.rmem_max=33554432
 *   sysctl -w net.core.wmem_max=33554432
 */
static void gnb_tune_udp_socket(int socketfd) {
    int bufsize = 8 * 1024 * 1024;   /* 8MB */
    setsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, (const char *)&bufsize, sizeof(bufsize));
    setsockopt(socketfd, SOL_SOCKET, SO_SNDBUF, (const char *)&bufsize, sizeof(bufsize));

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
    int flags = fcntl(socketfd, F_GETFL, 0);
    if ( flags >= 0 ) {
        fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);
    }
#endif

#ifdef _WIN32
    u_long nonblock = 1;
    ioctlsocket(socketfd, FIONBIO, &nonblock);
#endif
}

int gnb_bind_udp_socket_ipv4(int socketfd,const char *host, int port) {
    struct sockaddr_in svr_addr;
    memset(&svr_addr, 0, sizeof(struct sockaddr_in));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(port);
    if ( NULL != host ) {
        svr_addr.sin_addr.s_addr = inet_addr(host);
    } else {
        svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    int on = 1;
    setsockopt( socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on) );

    if ( bind(socketfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr_in)) < 0 ) {
        perror("bind");
        return -1;
    }

    /* 扩大缓冲区 + 非阻塞 */
    gnb_tune_udp_socket(socketfd);

    return 0;
}

int gnb_bind_udp_socket_ipv6(int socketfd,const char *host, int port) {
    struct sockaddr_in6 svr_addr;
    memset(&svr_addr,0, sizeof(struct sockaddr_in6));

    svr_addr.sin6_family = AF_INET6;
    svr_addr.sin6_port   = htons(port);

    if ( NULL != host ) {
        inet_pton( AF_INET6, host, &svr_addr.sin6_addr);
    } else {
        svr_addr.sin6_addr = in6addr_any;
    }

    int on;
    on = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,(const char *)&on, sizeof(on) );
    on = 1;
    setsockopt(socketfd, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on, sizeof(on) );
    if ( bind(socketfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr_in6))<0 ) {
        perror("bind");
        return -1;
    }

    /* 扩大缓冲区 + 非阻塞 */
    gnb_tune_udp_socket(socketfd);

    return 0;
}