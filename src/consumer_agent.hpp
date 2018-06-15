//
// Created by recolic on 18-5-17.
//

#ifndef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP

#include <provider_selector.hpp>
#define ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP_

#include <rlib/class_decorator.hpp>
#include <rlib/log.hpp>

#include <string>
#include <boost/asio.hpp>
#include <boost/coroutine2/coroutine.hpp>

#include <boost/asio/spawn.hpp>
#include <boost/beast/http.hpp>

extern rlib::logger rlog; // definition in src/main.cc

namespace consumer {

    class agent : rlib::noncopyable {
    public:
        agent() = delete;

        // Initialize provider_selector and it will connect to etcd now.
        agent(const std::string &etcd_addr_and_port, int threads = 1)
                : io_context(threads), selector(io_context, etcd_addr_and_port), threads(threads) {}

        // Launch http server and listen for requests from consumer.
        [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

    private:
        boost::asio::io_context io_context;
        provider_selector selector;
        int threads;

        void do_listen(boost::asio::ip::tcp::endpoint endpoint, boost::asio::yield_context yield);
        void do_session(boost::asio::ip::tcp::socket &&conn, boost::asio::yield_context yield);
        boost::beast::http::response<boost::beast::http::string_body>
        handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req,
                       boost::asio::yield_context &yield);
    };

} // namespace consumer

#endif //ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
