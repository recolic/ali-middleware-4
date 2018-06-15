//
// Created by recolic on 18-5-17.
//
// zxcpyp started working on 18-5-23.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP

#include <rlib/class_decorator.hpp>
#include <rlib/log.hpp>

#include <string>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/coroutine2/coroutine.hpp>

#include <boost/asio/spawn.hpp>
#include <boost/beast/http.hpp>

extern rlib::logger rlog; // definition in src/main.cc


namespace provider {

    class agent : rlib::noncopyable {
    public:
        agent() = delete;

        // Connect to etcd and register myself.
        agent(const std::string &etcd_addr_and_port,
         std::string &provider_addr, uint16_t provider_port, uint64_t request_id = 0x1);
        // Launch http server and listen for consumer_agent. Be caution that you should reuse connections.
        [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

      private:
        int threads;
        std::string provider_addr;
        uint16_t provider_port;
        std::atomic_uint64_t request_id;
        boost::asio::io_context io_context;

        // Listen consumer request(thread or coroutine). If get request, then call session_consumer().
        void listen_consumer(boost::asio::ip::tcp::endpoint endpoint, boost::asio::yield_context yield);

        // Do session with consumer, get HTTP request.
        void session_consumer(boost::asio::ip::tcp::socket &&conn, boost::asio::yield_context yield);

        // Handle HTTP request and do session with provider using dubbo
        boost::beast::http::response<boost::beast::http::string_body>
        handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req,
                       boost::asio::yield_context &yield);
        // Not sure if etcd do need heartbeat.
        [[noreturn]] void etcd_register_and_heartbeat(const std::string &etcd_addr_and_port);
        std::thread etcd_heartbeat_thread;
    };
}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP
