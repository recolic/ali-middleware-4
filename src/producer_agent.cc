//
// Created by recolic on 18-5-17.
//
// zxcpyp started working on 18-5-23.
//

#include <producer_agent.hpp>

#include <boost/beast/http/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/spawn.hpp>

using string_view = boost::beast::string_view;
namespace http = boost::beast::http;

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;

using namespace rlib::literals;


namespace producer {

    agent::agent(const std::string &etcd_addr_and_port) {
        // TODO: Connect to etcd, register myself. Launch heartbeat thread.
    }

    [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port) {
        // TODO: Launch http server, listen consumer request, and make dubbo request(thread or coroutine).
    }

    void listen_consumer() {
        // TODO: Launch http server, listen consumer request. If get request, then call sendto_producer().
    }

    void sendto_producer() {
        // TODO: Make dubbo request to producer. Then call listen_producer().
    }

    void listen_producer() {
        // TODO: Listen consumer request. If get request, then call sendto_consumer().
    }

    void sendto_consumer() {
        // TODO: Send HTTP request to consumer.
    }

    [[noreturn]] void etcd_register_and_heartbeat(const std::string &etcd_addr_and_port) {
        // TODO: Send heartbeat packet to etcd.
    }
}
