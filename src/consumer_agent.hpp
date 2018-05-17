//
// Created by recolic on 18-5-17.
//

#ifndef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP

#include <rlib/class_decorator.hpp>
#include <string>
#include <boost/asio.hpp>

class consumer_agent : rlib::noncopyable {
public:
    consumer_agent() = delete;

    consumer_agent(const std::string &etcd_addr, uint16_t etcd_port); // It's OK to connect to etcd server now.
    [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

private:

};


#endif //ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
