//
// Created by recolic on 18-5-18.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP

#include <boost_asio_quick_connect.hpp>
#include <etcd_service.hpp>
#include <conn_pool.hpp>
#include <string>
#include <list>

#include <rlib/log.hpp>
#include <rlib/macro.hpp>
#include <logger.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <rlib/scope_guard.hpp>

#ifdef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP_
#error consumer_agent.hpp must not be included before provider_selector.hpp.
#endif

namespace consumer {
    namespace http = boost::beast::http;

    // Manage connection to provider_agent servers. You must reuse these connections.
    class provider_info : rlib::noncopyable {
        using string_body = boost::beast::http::string_body;
    public:
        provider_info() = delete;

        // Connect to provider_agent and preserve connection.
        provider_info(boost::asio::io_context &ioContext, std::string addr, uint16_t port)
                : io_context(ioContext), hostname(addr),
                  pconns(new conn_pool_coro(ioContext, ioContext, addr, (uint16_t) port))
        {}

        // Auto-generated move constructor is ambiguous.
        provider_info(provider_info &&another)
                : io_context(another.io_context), hostname(std::move(another.hostname)),
                  pconns(std::move(another.pconns))
        {}

        inline boost::beast::http::response<string_body>
        async_request(boost::beast::http::request<string_body> &req, boost::asio::yield_context &yield) {
            boost::system::error_code ec;
            http::response<string_body> res;
            boost::beast::flat_buffer buffer;

            auto gen_500 = [&req] {
                http::response<http::string_body> res{http::status::internal_server_error, req.version()};
                res.set(http::field::server, "rHttp");
                res.set(http::field::content_type, "text/plain");
                res.body() = "server error";
                res.prepare_payload();
                return std::move(res);
            };

            auto borrowed_conn = pconns->borrow_one(yield);
            rlib_defer([&] { pconns->release_one(borrowed_conn); });
            boost::asio::ip::tcp::socket &conn = borrowed_conn->get();

            http::async_write(conn, req, yield[ec]);
            if (ec) {
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
                return gen_500();
            }
            http::async_read(conn, buffer, res, yield[ec]);
            if (ec) {
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
                return gen_500();
            }
            return std::move(res);
        }

        const std::string &get_host() const {
            return hostname;
        }

    private:
        boost::asio::io_context &io_context;
        std::unique_ptr<conn_pool_coro> pconns;
        std::string hostname;

        // Other data structure for burden level measurement.
    };

    class provider_selector : rlib::noncopyable {
    public:
        provider_selector() = delete;

#ifdef PROVIDER_SELECTOR_NO_USE_ETCD
        provider_selector(boost::asio::io_context &io_context, const std::string &etcd_addr_and_port)
                : io_context(io_context) {
            rlog.info("(fake_connect) connecting to etcd server {}."_format(etcd_addr_and_port));
            rlog.info("(fake_connect) initializing server list as {}:{}."_format(RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT));
            providers.emplace_back(io_context, RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT);
        }

        provider_info &query_once() {
            return *providers.begin();
        }
#else
        // Connect to etcd and fetch server list. You must use gRPC or REST API.
        provider_selector(boost::asio::io_context &io_context, const std::string &etcd_addr_and_port)
            : io_context(io_context), etcd(etcd_addr_and_port) {
            auto addr_list = etcd.get_list("server");
            for(const auto &_addr : addr_list) {
                if(_addr.empty())
                    continue;
                auto addr_and_port = _addr.split(':');
                if(addr_and_port.size() != 2)
                    throw std::runtime_error("Bad server_addr from etcd: `{}`."_format(_addr));
                providers.emplace_back(io_context, addr_and_port[0], addr_and_port[1].as<uint16_t>());
            }
            rlog.debug("Loaded {} providers."_format(providers.size()));
            if(providers.empty())
                throw std::runtime_error("No provider available. Unable to provide service...");
        }

        // Select one provider to query, do auto-balance here.
        // Determine which provider to query. Potential performance bottleneck here, MUST BE QUICK!
        provider_info &query_once();

    private:
        etcd_service etcd;
#endif


    private:
        std::vector<provider_info> providers;
        boost::asio::io_context &io_context;

    };

}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
