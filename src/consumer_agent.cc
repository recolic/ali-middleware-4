//
// Created by recolic on 18-5-17.
//

#include <boost_asio_quick_connect.hpp>
#include <consumer_agent.hpp>

consumer_agent::consumer_agent(const std::string &etcd_addr, uint16_t etcd_port) {
    auto sockServer = boost::asio::quick_connect(io_context, etcd_addr, etcd_port);
    // TODO: Call grpc and contact etcd.
}

void consumer_agent::listen(const std::string &listen_addr, uint16_t listen_port) {
    // TODO: Launch http server.
    // TODO: node balancer must be implemented somewhere.
}
