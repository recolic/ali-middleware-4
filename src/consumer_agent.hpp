//
// Created by recolic on 18-5-17.
//

#ifndef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP

#include <rlib/class_decorator.hpp>
#include <rlib/log.hpp>

#include <string>
#include <boost/asio.hpp>
#include <boost/coroutine2/coroutine.hpp>

#include <producer_selector.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/http.hpp>

extern rlib::logger rlog; // definition in src/main.cc

namespace consumer {

    class agent : rlib::noncopyable {
    public:
        agent() = delete;

        // Initialize producer_selector and it will connect to etcd now.
        agent(const std::string &etcd_addr_and_port, int threads = 1)
                : selector(etcd_addr_and_port), io_context(threads) {}

        // Launch http server and listen for requests from consumer.
        [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

    private:
        boost::asio::io_context io_context;
        producer_selector selector;
        void do_listen(boost::asio::ip::tcp::endpoint endpoint, boost::asio::yield_context yield);
        void do_session(boost::asio::ip::tcp::socket conn, boost::asio::yield_context yield);
        boost::beast::http::response handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req,
                                                    boost::asio::yield_context &yield);
    };

} // namespace consumer

#endif //ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
