//
// Created by recolic on 18-5-17.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP

#include <rlib/class_decorator.hpp>
#include <string>
#include <boost/asio.hpp>

namespace producer {

    class agent : rlib::noncopyable {
    public:
        agent() = delete;
        agent(const std::string &etcd_addr_and_port);
        // Launch http server and listen for consumer_agent. Be caution that you should reuse connections.
        [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

    private:
        boost::asio::io_context io_context;

        // Not sure if etcd do need heartbeat.
        [[noreturn]] void etcd_register_and_heartbeat(const std::string &etcd_addr_and_port);
        std::thread etcd_heartbeat_thread;
    };
}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP
