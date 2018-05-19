//
// Created by recolic on 18-5-18.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP

#include <consumer_agent.hpp>
#include <string>
#include <list>

namespace consumer {

    // Manage connection to producer_agent servers. You must reuse these connections.
    class producer_info : rlib::noncopyable {
    public:
        producer_info() = delete;
        producer_info(boost::asio::io_context &ioContext, const std::string &addr, uint16_t port);

    private:
        boost::asio::io_context &io_context;
        boost::asio::ip::tcp::socket conn;

        // Other data structure for burden level measurement.
    };

    class producer_selector {
    public:
        producer_selector() = delete;

        // Connect to etcd and fetch server list. You must use gRPC rather than REST API.
        producer_selector(const std::string &etcd_addr_and_port);

        // Select one producer to query, do auto-balance here.
        producer_info &query_once();

    private:
        std::list<producer_info> producers;
    };

}

#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
