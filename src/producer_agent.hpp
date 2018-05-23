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
#include <boost/asio.hpp>
#include <boost/coroutine2/coroutine.hpp>

#include <boost/asio/spawn.hpp>
#include <boost/beast/http.hpp>

extern rlib::logger rlog; // definition in src/main.cc

namespace producer {

    class agent : rlib::noncopyable {
    public:
        agent() = delete;

        // Connect to etcd and register myself.
        agent(const std::string &etcd_addr_and_port);
        // Launch http server and listen for consumer_agent. Be caution that you should reuse connections.
        [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

    private:
        boost::asio::io_context io_context;

        // TODO:
        // Four steps:
        // 1.Consumer Agent => Producer Agent: Receive HTTP (FORM)
        void listen_consumer();

        // 2.Producer Agent => Producer: Use DUBBO (JSON)
        void sendto_producer();

        // 3.Producer => Producer Agent: JSON
        void listen_producer();

        // 4.Producer Agent => Consumer Agent: Send HTTP (TEXT)
        void sendto_consumer();

        // Not sure if etcd do need heartbeat.
        [[noreturn]] void etcd_register_and_heartbeat(const std::string &etcd_addr_and_port);
        std::thread etcd_heartbeat_thread;
    };
}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_AGENT_HPP
