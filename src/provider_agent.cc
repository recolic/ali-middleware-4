//
// Created by recolic on 18-5-17.
//
// zxcpyp started working on 18-5-23.
//

#include <provider_agent.hpp>

#include <etcd_service.hpp>
#include <../lib/dubbo_client.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/spawn.hpp>

#include <logger.hpp>
#include <iostream>

namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;

using namespace rlib::literals;


namespace provider {

    agent::agent(const std::string &etcd_addr_and_port,
     std::string &provider_addr, uint16_t provider_port, uint64_t request_id)
     : provider_addr(provider_addr), provider_port(provider_port), request_id(request_id) {
        // TO/DO: Connect to etcd, register myself. Launch heartbeat thread.
        etcd_service etcd_service(etcd_addr_and_port);
        etcd_service.append("server", "{}:{}"_format(provider_addr, std::to_string(provider_port)));
    }

    [[noreturn]] void agent::listen(const std::string &listen_addr, uint16_t listen_port) {
        // TO/DO: Launch http server, listen consumer request. If get request, then call sendto_provider().
        tcp::endpoint ep(ip::make_address(listen_addr), listen_port);
        rlog.info("Launching http server (provider_agent) at {}:{}"_format(listen_addr, listen_port));
        asio::spawn(io_context, std::bind(&agent::listen_consumer, this, ep, std::placeholders::_1));
        io_context.run();
    }

    void agent::listen_consumer(tcp::endpoint ep, asio::yield_context yield) {
        // TO/DO: Listen consumer request(thread or coroutine). If get request, then call session_consumer().
        boost::system::error_code ec;
        tcp::acceptor acceptor(io_context);

        acceptor.open(ep.protocol(), ec);
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.set_option(asio::socket_base::reuse_address(true));
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.bind(ep, ec);
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) ON_BOOST_FATAL(ec);

        while(true) {
            tcp::socket conn(io_context);
            acceptor.async_accept(conn, yield[ec]);
            if (ec)
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
            else
                asio::spawn(io_context, std::bind(&agent::session_consumer, this, std::bind(static_cast<tcp::socket&&(&)(tcp::socket&)>(std::move<tcp::socket&>), std::move(conn)), std::placeholders::_1));
        }
    }

    void agent::session_consumer(tcp::socket &&conn, asio::yield_context yield) {
        // TO/DO: Do session with consumer
        boost::system::error_code ec;
        boost::beast::flat_buffer buffer;
        rlog.debug("session launched.");

        while (true) {
            http::request<http::string_body> req;
            http::async_read(conn, buffer, req, yield[ec]);
            if (ec == http::error::end_of_stream)
                break;
            if (ec) ON_BOOST_FATAL(ec);

            auto response = handle_request(std::move(req), yield);

            http::async_write(conn, response, yield[ec]);

            if (!response.keep_alive())
                break;
        }

        conn.shutdown(tcp::socket::shutdown_send, ec);
        if (ec) ON_BOOST_FATAL(ec);
    }

    http::response<http::string_body> agent::handle_request(http::request<http::string_body> &&req, asio::yield_context &yield) {

        // Return 400 if not GET
        if (req.method() != http::verb::get) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "bad request";
            res.prepare_payload();
            return std::move(res);
        }

        //TO/DO: Dubbo client
        auto kv_str = rlib::string(req.body()).split("&");
        kv_serializer::kv_list_t kv_list;
        for (int i = 0; i < kv_str.size(); i++) {
            auto kv_pair = rlib::string(kv_str[i]).split("=");
            kv_list.push_back(std::make_pair(kv_pair[0], kv_pair[1]));
        }
        dubbo_client dubbo(provider_addr, provider_port);
        auto result = dubbo.request(kv_list, request_id);
        request_id += 1;

        rlog.debug("List payload:");
        for (auto &ele : result.payload) {
            rlib::print("{{}, {}}"_format(ele.first, ele.second));
        }
        rlib::println();

        if (result.status == dubbo_client::status_t::OK) {
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "Hello world!";
            res.prepare_payload();
            return std::move(res);
        }
        else {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "Dubbo client error!";
            res.prepare_payload();
            return std::move(res);
        }
    }

    [[noreturn]] void agent::etcd_register_and_heartbeat(const std::string &etcd_addr_and_port) {
        // TODO: Send heartbeat packet to etcd.
    }
}

