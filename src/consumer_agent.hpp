//
// Created by recolic on 18-5-17.
//

#ifndef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
#define ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP

#include <rlib/class_decorator.hpp>
#include <string>
#include <boost/asio.hpp>
#include <producer_selector.hpp>

namespace consumer {

    class agent : rlib::noncopyable {
    public:
        agent() = delete;

        // Initialize producer_selector and it will connect to etcd now.
        agent(const std::string &etcd_addr_and_port)
                : selector(etcd_addr_and_port) {}

        // Launch http server and listen for requests from consumer.
        [[noreturn]] void listen(const std::string &listen_addr, uint16_t listen_port);

    private:
        boost::asio::io_context io_context;
        producer_selector selector;
    };

} // namespace consumer

#endif //ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP
