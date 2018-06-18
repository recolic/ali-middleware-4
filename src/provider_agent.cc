//
// Created by recolic on 18-5-17.
//
// zxcpyp started working on 18-5-23.
//

#include <provider_agent.hpp>

#include <etcd_service.hpp>
#include <dubbo_client.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/endian/endian.hpp>

#include <logger.hpp>
#include <iostream>

namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;

using namespace rlib::literals;


namespace provider {

    agent::agent(const std::string &etcd_addr_and_port, const std::string &my_addr, uint16_t listen_port,
                 const std::string &provider_addr, uint16_t provider_port)
            : provider_addr(provider_addr), provider_port(provider_port),
              dubbo(io_context, provider_addr, provider_port) {
        // TO/DO: Connect to etcd, register myself. Launch heartbeat thread.
        etcd_service etcd_service(etcd_addr_and_port);
        etcd_service.append("server", "{}:{}"_format(my_addr, listen_port));
    }

    [[noreturn]] void agent::listen(const std::string &listen_addr, uint16_t listen_port) {
        // TO/DO: Launch http server, listen consumer request. If get request, then call sendto_provider().
        tcp::endpoint ep(ip::make_address(listen_addr), listen_port);
        rlog.info("Launching http server (provider_agent) at {}:{}"_format(listen_addr, listen_port));
        rlog.debug("test debugging tool");
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

        while (true) {
            tcp::socket conn(io_context);
            acceptor.async_accept(conn, yield[ec]);
            if (ec)
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
            else
                asio::spawn(io_context, std::bind(&agent::session_consumer, this, std::bind(
                        static_cast<tcp::socket &&(&)(tcp::socket &)>(std::move<tcp::socket &>), std::move(conn)),
                                                  std::placeholders::_1));
        }
    }

    void agent::session_consumer(tcp::socket &&conn, asio::yield_context yield) {
        // TO/DO: Do session with consumer
        boost::system::error_code ec;
        boost::beast::flat_buffer buffer;

        try {
            while (true) {
                uint32_t pkg_len = 0;
                asio::async_read(conn, asio::buffer(&pkg_len, sizeof(pkg_len)), yield[ec]);
                pkg_len = boost::endian::big_to_native(pkg_len);
                char *req_buffer = new char[pkg_len+1]{0};
                rlib_defer(std::bind(std::free, req_buffer));
                auto _size = asio::async_read(conn, asio::buffer(req_buffer, pkg_len), yield[ec]);
                if (ec && ec != boost::asio::error::eof) ON_BOOST_FATAL(ec);
                rlog.debug("read_ `{}`, pkg_len is {}"_format(req_buffer, pkg_len));
                if (_size == 0) continue;

                auto req_args = rlib::string(req_buffer).split('\n');
                if(req_args.size() != 4) {
                    rlog.error("invalid request `{}` dropped."_format(req_buffer));
                    continue;
                }
                auto result = dubbo.async_request(req_args[0], req_args[1], req_args[2], req_args[3], yield);
                if (result.status != dubbo_client::status_t::OK)
                    throw std::runtime_error("dubbo not ok.");

                rlog.debug("returning `{}`"_format(result.value));
                pkg_len = boost::endian::native_to_big(result.value.size());
                //asio::async_write(conn, asio::buffer(&pkg_len, sizeof(pkg_len)), yield[ec]);
                asio::async_write(conn, asio::buffer(result.value), yield[ec]);
            }
        }
        catch (std::exception &ex) {
            rlog.error("Exception caught at provider session: {}"_format(ex.what()));
        }

        conn.shutdown(tcp::socket::shutdown_send, ec);
        if (ec) ON_BOOST_FATAL(ec);
    }

    http::response<http::string_body>
    [[deprecated]] agent::handle_request(http::request<http::string_body> &&req, asio::yield_context &yield) {
        rlog.debug("handle req");

        // Return 400 if not POST
        if (req.method() != http::verb::post) {
            rlog.error("Warning: Non-POST request received. request dropped.");
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "bad request";
            res.prepare_payload();
            return std::move(res);
        }

        //TO/DO: Dubbo client
        auto kv_str_array = rlib::string(req.body()).split("&");
        std::array<std::string, 4> dubbo_rpc_args;
        for (auto &&kv_str : kv_str_array) {
            auto pos = kv_str.find('=');
            if (pos == std::string::npos) {
                rlog.error("bad kv_Str `{}` skipped"_format(kv_str));
                continue;
            }
            auto key = kv_str.substr(0, pos);
            auto val = kv_str.substr(pos + 1);

            if (key == "interface")
                dubbo_rpc_args[0] = std::move(val);
            else if (key == "method")
                dubbo_rpc_args[1] = std::move(val);
            else if (key == "parameterTypesString")
                dubbo_rpc_args[2] = std::move(val);
            else if (key == "parameter")
                dubbo_rpc_args[3] = std::move(val);
            else
                rlog.error("Warning: got a request with bad body `{}`"_format(req.body()));
        }
        dubbo_rpc_args[2] = rlib::string(std::move(dubbo_rpc_args[2])).replace("%2F", "/").replace("%3B", ";");
        for (auto &s : dubbo_rpc_args) {
            if (s.empty()) {
                rlog.error("Exception detail: par is `{}`"_format(req.body()));
                throw std::runtime_error("Required http argument not provided.");
            }
        }

        auto result = dubbo.async_request(dubbo_rpc_args[0], dubbo_rpc_args[1], dubbo_rpc_args[2], dubbo_rpc_args[3], yield);

        rlog.debug("Dubbo request result is `{}`"_format(result.value));

        if (result.status == dubbo_client::status_t::OK) {
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = result.value;
            res.prepare_payload();
            return std::move(res);
        } else {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "Dubbo error.";
            res.prepare_payload();
            return std::move(res);
        }
    }

    [[noreturn]] void agent::etcd_register_and_heartbeat(const std::string &etcd_addr_and_port) {
        // TODO: Send heartbeat packet to etcd.
    }

}

