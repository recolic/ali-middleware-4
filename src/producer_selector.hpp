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

#ifdef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP_
#error consumer_agent.hpp must not be included before producer_selector.hpp.
#endif

namespace consumer {

    // Manage connection to producer_agent servers. You must reuse these connections.
    class producer_info : rlib::noncopyable {
        using string_body = boost::beast::http::string_body;
    public:
        producer_info() = delete;

        // Connect to producer_agent and preserve connection.
        producer_info(boost::asio::io_context &ioContext, std::string addr, uint16_t port)
                : io_context(ioContext), hostname(addr), pconns(new conn_pool(ioContext, addr, (uint16_t) port))
        {}

        // Auto-generated move constructor is ambiguous.
        producer_info(producer_info &&another)
                : io_context(another.io_context), hostname(std::move(another.hostname)),
                  pconns(std::move(another.pconns))
        {}

        inline boost::beast::http::response<string_body>
        async_request(boost::beast::http::request<string_body> &req, boost::asio::yield_context &yield) {
            boost::system::error_code ec;
            boost::beast::http::response<string_body> res;
            boost::asio::ip::tcp::socket &conn = pconns->borrow_one()->get();
            boost::beast::http::async_write(conn, req, yield[ec]);
            if (ec) {
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
                return std::move(res);
            }
            boost::beast::flat_buffer buffer;
            boost::beast::http::async_read(conn, buffer, res, yield[ec]);
            if (ec) RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
            return std::move(res);
        }

        const std::string &get_host() const {
            return hostname;
        }

    private:
        boost::asio::io_context &io_context;
        std::unique_ptr<conn_pool> pconns;
        std::string hostname;

        // Other data structure for burden level measurement.
    };

    class producer_selector : rlib::noncopyable {
    public:
        producer_selector() = delete;

#ifdef PRODUCER_SELECTOR_UNFINISHED
        producer_selector(boost::asio::io_context &io_context, const std::string &etcd_addr_and_port)
                : io_context(io_context) {
            rlog.info("(fake_connect) connecting to etcd server {}."_format(etcd_addr_and_port));
            rlog.info("(fake_connect) initializing server list as {}:{}."_format(RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT));
            producers.emplace_back(io_context, RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT);
        }

        producer_info &query_once() {
            return *producers.begin();
        }
#else
        // Connect to etcd and fetch server list. You must use gRPC or REST API.
        producer_selector(boost::asio::io_context &io_context, const std::string &etcd_addr_and_port)
            : etcd(etcd_addr_and_port) {
            auto producer_list = etcd.get_list();
            // Construct this->producers from information in producer_list. Burden auto-balance data structure must also be added to producer_info.
        }

        // Select one producer to query, do auto-balance here.
        // Determine which producer to query. Potential performance bottleneck here, MUST BE QUICK!
        producer_info &query_once();

    private:
        etcd_service etcd;
#endif


    private:
        std::list<producer_info> producers;
        boost::asio::io_context &io_context;

    };

}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
