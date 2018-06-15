//
// Created by recolic on 18-5-19.
//

#ifndef ALI_MIDDLEWARE_AGENT_HTTP_FORWARDER_HPP
#define ALI_MIDDLEWARE_AGENT_HTTP_FORWARDER_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

#include <provider_selector.hpp>

namespace consumer {

// This http forwarder must be implemented with multi-threading and coroutine.
    class [[deprecated]] http_forwarder : rlib::noncopyable {
    public:
        http_forwarder() = delete;

        http_forwarder(provider_selector &selector) : selector(selector) {}

        [[noreturn]] void serve(const std::string &listen_addr, uint16_t listen_port/*etcd arg?*/) {

        }

    private:
        boost::asio::io_context io_context;
        provider_selector selector;
    };

}

namespace provider {
    class [[deprecated]] http_dubbo_forwarder {
    public:
        [[noreturn]] void serve() {

        }
    };
}

#endif //ALI_MIDDLEWARE_AGENT_HTTP_FORWARDER_HPP
