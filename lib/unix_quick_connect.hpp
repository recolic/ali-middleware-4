#ifndef _ALI_MIDDLEWARE_AGENT_UNIX_QUICK_CONNECT_HPP
#define _ALI_MIDDLEWARE_AGENT_UNIX_QUICK_CONNECT_HPP 1

#include <rlib/sys/fd.hpp>
#include <sys/socket.h>
#include <netdb.h>
#include <logger.hpp>
#include <rlib/scope_guard.hpp>

namespace rlib {
    inline fd unix_quick_listen(const std::string &addr, uint16_t port) {
        addrinfo *psaddr;
        addrinfo hints{0};
        fd listenfd;

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
        auto _ = getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &hints, &psaddr);
        if (_ != 0) sysdie("Failed to getaddrinfo. returnval={}, check `man getaddrinfo`'s return value."_format(_));

        bool success = false;
        for (addrinfo *rp = psaddr; rp != nullptr; rp = rp->ai_next) {
            listenfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (listenfd == -1)
                continue;
            int reuse = 1;
            if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(int)) < 0)
                sysdie("setsockopt(SO_REUSEADDR) failed");
            if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse, sizeof(int)) < 0)
                sysdie("setsockopt(SO_REUSEPORT) failed");
            if (bind(listenfd, rp->ai_addr, rp->ai_addrlen) == 0) {
                success = true;
                break;
            }
            close(listenfd);
        }
        if (!success) sysdie("Failed to bind {}:{}."_format(addr, port));

        if (-1 == ::listen(listenfd, 16)) sysdie("listen failed.");

        rlib_defer([psaddr] { freeaddrinfo(psaddr); });
        return listenfd;
    }

    inline fd unix_quick_connect(const std::string &addr, uint16_t port) {
        addrinfo *paddr;
        addrinfo hints{0};
        fd sockfd;

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        auto _ = getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &hints, &paddr);
        if (_ != 0)
            sysdie("getaddrinfo failed. Check network connection to {}:{}; returnval={}, check `man getaddrinfo`'s return value."_format(
                    addr.c_str(), port, _));
        rlib_defer([paddr] { freeaddrinfo(paddr); });

        bool success = false;
        for (addrinfo *rp = paddr; rp != NULL; rp = rp->ai_next) {
            sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sockfd == -1)
                continue;
            int reuse = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(int)) < 0)
                sysdie("setsockopt(SO_REUSEADDR) failed");
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse, sizeof(int)) < 0)
                sysdie("setsockopt(SO_REUSEPORT) failed");
            if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
                success = true;
                break; /* Success */
            }
            close(sockfd);
        }
        if (!success) sysdie("Failed to connect to any of these addr.");

        return sockfd;
    }
}

#endif //_ALI_MIDDLEWARE_AGENT_UNIX_QUICK_CONNECT_HPP
