//
// Created by recolic on 18-5-18.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP

#include <boost_asio_quick_connect.hpp>
#include <string>
#include <list>

#include <rlib/log.hpp>
#include <rlib/macro.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#ifdef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP_
#error consumer_agent.hpp must not be included before producer_selector.hpp.
#endif

using rlib::literals::operator ""_format;
extern rlib::logger rlog;

namespace consumer {

    // Manage connection to producer_agent servers. You must reuse these connections.
    class producer_info : rlib::noncopyable {
        using string_body = boost::beast::http::string_body;
    public:
        producer_info() = delete;

        // Connect to producer_agent and preserve connection.
        producer_info(boost::asio::io_context &ioContext, const std::string &addr, uint16_t port)
                : io_context(ioContext), conn(boost::asio::quick_connect(ioContext, addr, port))
        {}

        // Auto-generated move constructor is ambiguous.
        producer_info(producer_info &&another)
                : io_context(another.io_context), conn(std::move(another.conn)), buffer(std::move(another.buffer))
        {}

        ~producer_info() {
            conn.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        }

        inline boost::beast::http::response<string_body>
        async_request(boost::beast::http::request<string_body> &req, boost::asio::yield_context &handler) {
            boost::beast::http::async_write(conn, req, handler);
            boost::beast::http::response<string_body> res;
            boost::beast::http::async_read(conn, buffer, res, handler);
            return std::move(res);
        }

    private:
        boost::asio::io_context &io_context;
        boost::asio::ip::tcp::socket conn;

        boost::beast::flat_buffer buffer;

        // Other data structure for burden level measurement.
    };

    class producer_selector {
    public:
        producer_selector() = delete;

#ifdef PRODUCER_SELECTOR_UNFINISHED

        producer_selector(const std::string &etcd_addr_and_port) {
            rlog.info("(fake_connect) connecting to etcd server {}."_format(etcd_addr_and_port));
            rlog.info("(fake_connect) initializing server list as {}:{}."_format(RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT));
            producers.push_back(producer_info(io_context, RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT));
        }

        producer_info &query_once() {
            return *producers.begin();
        }

#else
        // Connect to etcd and fetch server list. You must use gRPC or REST API.
        producer_selector(const std::string &etcd_addr_and_port);

        // Select one producer to query, do auto-balance here.
        producer_info &query_once();
#endif


    private:
        std::list<producer_info> producers;
        boost::asio::io_context io_context;
    };

}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
