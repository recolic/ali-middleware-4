//
// Created by recolic on 18-5-17.
//

#ifndef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP

#include <provider_selector.hpp>
#define ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP_

#include <sys/epoll.h>
#include <rlib/class_decorator.hpp>
#include <rlib/log.hpp>

#include <string>

extern rlib::logger rlog; // definition in src/main.cc

namespace consumer {

    class agent : rlib::noncopyable {
    public:
        agent() = delete;

        // Initialize provider_selector and it will connect to etcd now.
        agent(const std::string &etcd_addr_and_port, int threads = 1)
                : epollfd(epoll_create1(0)), selector(epollfd, etcd_addr_and_port), threads(threads) {
            if (epollfd == -1)
                sysdie("epoll_create1");
        }

        // Launch http server and listen for requests from consumer.
        [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

    private:
        fd epollfd; // Warning: epollfd must be initialized before selector!
        provider_selector selector;
        int threads;

        void do_epoll_proc(fd listenfd);
        void on_readfd_available(fd connfd);

        std::unordered_map<fd, fd> who_serve_who;

        auto epoll_event_new(fd _fd, uint32_t _events) {
            epoll_event ev;
            ev.events = _events;
            ev.data.fd = _fd;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, _fd, &ev) == -1)
                sysdie("epoll_ctl: fd");
        }

        auto epoll_event_del(epoll_event &ev) {
            if (epoll_ctl(epollfd, EPOLL_CTL_DEL, ev.data.fd, &ev) == -1)
                sysdie("epoll_ctl: fd");
        }
    };

} // namespace consumer

#endif //ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
