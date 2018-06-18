//
// Created by recolic on 18-5-17.
//

#include <consumer_agent.hpp>

#include <functional>
#include <logger.hpp>
#include <rlib/sys/sio.hpp>
#include <unix_quick_connect.hpp>

using namespace rlib::literals;

namespace consumer {

    [[noreturn]] void agent::listen(const std::string &listen_addr, uint16_t listen_port) {
        do_epoll_proc(rlib::unix_quick_listen(listen_addr, listen_port));
    }

#define RAGENT_EPOLL_MAX_EV 16

    void agent::do_epoll_proc(fd listenfd) {
        epoll_event events[RAGENT_EPOLL_MAX_EV];
        int nfds;

        epoll_event_new(listenfd, EPOLLIN);

        for (;;) {
            nfds = epoll_wait(epollfd, events, RAGENT_EPOLL_MAX_EV, -1);
            if (nfds == -1)
                sysdie("epoll_wait");
            rlog.debug("nfds={}"_format(nfds));

            for (int n = 0; n < nfds; ++n) {
                if (events[n].data.fd == listenfd) {
                    // Must accept
                    fd conn_sock = accept(listenfd, nullptr, nullptr);
                    if (conn_sock == -1)
                        sysdie("accept failed");
                    //rlib::impl::MakeNonBlocking(conn_sock);
                    epoll_event_new(conn_sock, EPOLLIN | EPOLLRDHUP);
                    consumer_conns.insert(conn_sock);
                } else {
                    auto &ev = events[n];
                    if (ev.events & EPOLLIN) {
                        try {
                            on_readfd_available(ev.data.fd);
                        }
                        catch (std::exception &e) {
                            rlog.error("caught exception `{}` from epoll loop."_format(e.what()));
                            if (consumer_conns.find(ev.data.fd) != consumer_conns.end()) {
                                epoll_event_del(ev);
                                close(ev.data.fd);
                                continue;
                            }
                        }
                    }
                    if (ev.events & EPOLLRDHUP) {
                        // Closed connection.
                        if (consumer_conns.find(ev.data.fd) != consumer_conns.end()) {
                            rlog.debug("conn {} closed."_format(ev.data.fd));
                            epoll_event_del(ev);
                            close(ev.data.fd);
                        }
                    }
                }
            }
        }

    }

    void agent::on_readfd_available(fd conn) {
        void *buffer = malloc(1024);
        rlib_defer([&buffer] { free(buffer); });
        RDEBUG_CURR_TIME_VAR(time1);
        auto _size = rlib::sockIO::recvall_ex(conn, &buffer, 1024, 0);
        if (_size == 0)
            return;
        RDEBUG_CURR_TIME_VAR(time2);
        RDEBUG_LOG_TIME_DIFF(time2, time1, "consumer epoll read time");
        std::string payload((char *) buffer, _size);
        rlog.debug("epoll read `{}`"_format(payload));

        static const std::string resp_400 = "HTTP/1.1 400 Bad Request\r\nServer: rHttp\r\nContent-Length: 3\r\nContent-Type: text/plain\r\n\r\n400";

        if (payload.substr(0, 4) == "POST") {
            // consumer is writing something
            auto body = *rlib::string(payload).split("\r\n").rbegin();

            auto server_fd = selector.query_once()->request(conn, body);
            who_serve_who[server_fd] = conn;
        } else if (payload.substr(0, 3) == "GET") {
            rlib::sockIO::sendn_ex(conn, resp_400.data(), resp_400.size(), MSG_NOSIGNAL);
            rlog.error("invalid req get");
            throw std::runtime_error("invalid req get");
        } else if (payload.find_first_not_of("-+1234567890") == std::string::npos) {
            // provider agent is writing correct result
            auto consumer_fd = who_serve_who.at(conn);
            who_serve_who.erase(conn);
            rlog.debug("got query from server. routing to consumer {}"_format(consumer_fd));
            auto to_write = "HTTP/1.1 200 OK\r\nServer: rHttp\r\nContent-Length: {}\r\nContent-Type: text/plain\r\n\r\n{}"_format(
                    payload.size(), payload);
            rlib::sockIO::sendn_ex(consumer_fd, to_write.data(), to_write.size(), MSG_NOSIGNAL);
        } else {
            rlog.error("Unknown packet `{}` dropped."_format(payload));
        }
    }

}
