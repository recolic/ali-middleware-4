//
// Created by recolic on 18-5-17.
//
// zxcpyp started working on 18-5-23.
//

#include <provider_agent.hpp>

#include <sys/socket.h>
#include <sys/epoll.h> 

#include <etcd_service.hpp>
#include <dubbo_client.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/spawn.hpp>

#include <logger.hpp>
#include <iostream>

#define FD_MAX 128
#define EPOLL_MAX 10000

namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;

using namespace rlib::literals;


namespace provider {

    agent::agent(const std::string &etcd_addr_and_port, const std::string &my_addr, uint16_t listen_port,
                 const std::string &provider_addr, uint16_t provider_port)
            : provider_addr(provider_addr), provider_port(provider_port),
              dubbo(io_context, provider_addr, provider_port) {
        // TO/DO: Connect to etcd, register myself. Launch heartbeat thread.
        etcd_service etcd_service(etcd_addr_and_port);
        etcd_service.append("server", "{}:{}"_format(my_addr, listen_port));
    }

    [[noreturn]] void agent::listen(const std::string &listen_addr, uint16_t listen_port) {
        // TO/DO: Launch http server, listen consumer request. If get request, then call sendto_provider().
        /*tcp::endpoint ep(ip::make_address(listen_addr), listen_port);
        rlog.info("Launching http server (provider_agent) at {}:{}"_format(listen_addr, listen_port));
        rlog.debug("test debugging tool");
        asio::spawn(io_context, std::bind(&agent::listen_consumer, this, ep, std::placeholders::_1));
        io_context.run();*/
        struct epoll_event event;
        struct epoll_event wait_event;

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in sockaddr;
        bzero(&sockaddr, sizeof(sockaddr));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(listen_port);
        sockaddr.sin_addr = ip::make_address(listen_addr);

        bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

        int fd[FD_MAX];
        memset(fd,-1, sizeof(fd));  
        fd[0] = sockfd;

        int epfd = epoll_create(EPOLL_MAX);
        if (epfd == -1)
        {
            rlog.error("epoll create error");
            exit(1);
        }

        event.data.fd = sockfd;
        event.events = EPOLLIN;

        int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
        if (ret == -1) {
            rlog.error("epoll ctl error");
            exit(1);
        }

        int i = 0, maxi = 0;
        for (;;) {
            ret = ret = epoll_wait(epfd, &wait_event, maxi + 1, -1);

            if ((sockfd = wait_event.data.fd) && (EPOLLIN == wait_event.events & EPOLLIN)) {
                struct sockaddr_in client_addr;  
                int client_len = sizeof(cli_addr);
                int conn = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);

                for (i = 1; i < FD_MAX; i++) {
                    if (fd[i] < 0) {
                        fd[i] = conn;
                        event.data.fd = conn;
                        event.events = EPOLLIN;

                        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &event);
                        if (ret == -1) {
                            rlog.error("epoll ctl error");
                            exit(1);
                        }
                        break;
                    }
                }
                if (i > maxi)
                    maxi = i;
                if (--ret <= 0)
                    continue;
            }
            for (i = 1; i <= maxi; i++) {
                if (fd[i] < 0)
                    continue;
                if ((fd[i] == wait_event.data.fd) & (EPOLLIN == wait_event.events & (EPOLLIN | EPOLLERR))) {
                    int len = 0;
                    char req_buffer[2048] = {0d};

                    if ((len = recv(fd[i], buf, sizeof(buf), 0)) < 0) {
                        if (errno == ECONNRESET)
                        {
                            close(fd[i]);
                            fd[i] = -1;
                        }
                        else
                        {
                            rlog.error("Read error");
                        }
                    }
                    else if (len == 0) {
                        close(fd[i]);
                        fd[i] = -1;
                    }
                    else {
                        auto req_args = rlib::string(req_buffer).split('\n');
                        auto result = dubbo.async_request(req_args[0], req_args[1], req_args[2], req_args[3], yield);
                        if (result.status != dubbo_client::status_t::OK)
                        throw std::runtime_error("dubbo not ok.");
                        send(fd[i], result.value, sizeof(result.value), 0);
                    }
                }
            }
        }
    }

    void agent::listen_consumer(tcp::endpoint ep, asio::yield_context yield)
    {
        // TO/DO: Listen consumer request(thread or coroutine). If get request, then call session_consumer().
        boost::system::error_code ec;
        tcp::acceptor acceptor(io_context);

        acceptor.open(ep.protocol(), ec);
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.set_option(asio::socket_base::reuse_address(true));
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.bind(ep, ec);
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) ON_BOOST_FATAL(ec);

        while (true) {
            tcp::socket conn(io_context);
            acceptor.async_accept(conn, yield[ec]);
            if (ec)
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
            else
                asio::spawn(io_context, std::bind(&agent::session_consumer, this, std::bind(
                        static_cast<tcp::socket &&(&)(tcp::socket &)>(std::move<tcp::socket &>), std::move(conn)),
                                                  std::placeholders::_1));
        }
    }

    void agent::session_consumer(tcp::socket &&conn, asio::yield_context yield) {
        // TO/DO: Do session with consumer
        boost::system::error_code ec;
        boost::beast::flat_buffer buffer;

        try {
            while (true) {
                //http::request<http::string_body> req;
                char req_buffer[2048] = {0};
                RDEBUG_CURR_TIME_VAR(time1);
                //http::async_read(conn, buffer, req, yield[ec]);
                // TODO TODO URGENT: This statement is slow. (takes 50ms-130ms or so)
                conn.async_read_some(asio::buffer(req_buffer, 2048), yield[ec]);
                //if (ec == http::error::end_of_stream)
                //break;
                if (ec && ec != boost::asio::error::eof) ON_BOOST_FATAL(ec);
                rlog.debug("read_ `{}`"_format(req_buffer));

                RDEBUG_CURR_TIME_VAR(time2);
                //auto response = handle_request(std::move(req), yield);
                auto req_args = rlib::string(req_buffer).split('\n');
                auto result = dubbo.async_request(req_args[0], req_args[1], req_args[2], req_args[3], yield);
                if (result.status != dubbo_client::status_t::OK)
                    throw std::runtime_error("dubbo not ok.");
                // TODO: may optimize: 1-2 ms
                RDEBUG_CURR_TIME_VAR(time3);

                rlog.debug("returning `{}`"_format(result.value));
                asio::async_write(conn, asio::buffer(result.value), yield[ec]);
                //http::async_write(conn, response, yield[ec]);
                RDEBUG_CURR_TIME_VAR(time4);
                RDEBUG_LOG_TIME_DIFF(time2, time1, "read_time");
                RDEBUG_LOG_TIME_DIFF(time3, time2, "handle_req_time");
                RDEBUG_LOG_TIME_DIFF(time4, time3, "write_time");

                //if (!response.keep_alive())
                //    break;
            }
        }
        catch (std::exception &ex) {
            rlog.error("Exception caught at provider session: {}"_format(ex.what()));
        }

        conn.shutdown(tcp::socket::shutdown_send, ec);
        if (ec) ON_BOOST_FATAL(ec);
    }

    http::response<http::string_body>
    agent::handle_request(http::request<http::string_body> &&req, asio::yield_context &yield) {
        rlog.debug("handle req");

        // Return 400 if not POST
        if (req.method() != http::verb::post) {
            rlog.error("Warning: Non-POST request received. request dropped.");
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "bad request";
            res.prepare_payload();
            return std::move(res);
        }

        //TO/DO: Dubbo client
        auto kv_str_array = rlib::string(req.body()).split("&");
        std::array<std::string, 4> dubbo_rpc_args;
        for (auto &&kv_str : kv_str_array) {
            auto pos = kv_str.find('=');
            if (pos == std::string::npos) {
                rlog.error("bad kv_Str `{}` skipped"_format(kv_str));
                continue;
            }
            auto key = kv_str.substr(0, pos);
            auto val = kv_str.substr(pos + 1);

            if (key == "interface")
                dubbo_rpc_args[0] = std::move(val);
            else if (key == "method")
                dubbo_rpc_args[1] = std::move(val);
            else if (key == "parameterTypesString")
                dubbo_rpc_args[2] = std::move(val);
            else if (key == "parameter")
                dubbo_rpc_args[3] = std::move(val);
            else
                rlog.error("Warning: got a request with bad body `{}`"_format(req.body()));
        }
        dubbo_rpc_args[2] = rlib::string(std::move(dubbo_rpc_args[2])).replace("%2F", "/").replace("%3B", ";");
        for (auto &s : dubbo_rpc_args) {
            if (s.empty()) {
                rlog.error("Exception detail: par is `{}`"_format(req.body()));
                throw std::runtime_error("Required http argument not provided.");
            }
        }

        auto result = dubbo.async_request(dubbo_rpc_args[0], dubbo_rpc_args[1], dubbo_rpc_args[2], dubbo_rpc_args[3], yield);

        rlog.debug("Dubbo request result is `{}`"_format(result.value));

        if (result.status == dubbo_client::status_t::OK) {
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = result.value;
            res.prepare_payload();
            return std::move(res);
        } else {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "Dubbo error.";
            res.prepare_payload();
            return std::move(res);
        }
    }

    [[noreturn]] void agent::etcd_register_and_heartbeat(const std::string &etcd_addr_and_port) {
        // TODO: Send heartbeat packet to etcd.
    }

}

