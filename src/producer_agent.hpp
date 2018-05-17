//
// Created by recolic on 18-5-17.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP

#include <rlib/class_decorator.hpp>
#include <string>
#include <boost/asio.hpp>

class producer_agent : rlib::noncopyable {
public:
    producer_agent() = delete;

    producer_agent(const std::string &etcd_addr, uint16_t etcd_port); // It's OK to connect to etcd server now.
    [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

private:
    boost::asio::ip::tcp::socket sockEtcd;

};


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP
