//
// Created by recolic on 18-5-17.
//

#include <consumer_agent.hpp>

#include <boost/beast/http/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/spawn.hpp>

#include <functional>
#include <logger.hpp>

using string_view = boost::beast::string_view;
namespace http = boost::beast::http;

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;

using namespace rlib::literals;

namespace consumer {

    [[noreturn]] void agent::listen(const std::string &listen_addr, uint16_t listen_port) {
        tcp::endpoint endpoint(ip::make_address(listen_addr), listen_port);

        static_assert(std::is_same<decltype(std::bind(&agent::do_listen, this, endpoint, std::placeholders::_1)(
                *(asio::yield_context *) nullptr)),
                void>::value, "Error on deducting type for asio::spawn arg2.");
        rlog.info("Launching http server (consumer_agent) at {}:{}"_format(listen_addr, listen_port));
        asio::spawn(io_context, std::bind(&agent::do_listen, this, std::move(endpoint), std::placeholders::_1));

        std::vector<std::thread> v;
        v.reserve((size_t) threads - 1);
        for (auto i = threads - 1; i > 0; --i)
            v.emplace_back([this] { this->io_context.run(); });
        io_context.run();
    }

    void agent::do_listen(tcp::endpoint endpoint, asio::yield_context yield) {
        boost::system::error_code ec;

        tcp::acceptor acceptor(io_context);
        acceptor.open(endpoint.protocol(), ec);
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.set_option(asio::socket_base::reuse_address(true));
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.bind(endpoint, ec);
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) ON_BOOST_FATAL(ec);

        while (true) {
            tcp::socket conn(io_context);
            acceptor.async_accept(conn, yield[ec]);
            if (ec)
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
            else {
                /*
                 * std::bind doesn't allow rvalue (ref) so this version doesn't work.
                 * auto _fdebug = std::bind(&agent::do_session, this, std::move(conn), std::placeholders::_1);
                 * typeof(_fdebug) is std::_Bind<void (consumer::agent::*(consumer::agent *, socket, std::_Placeholder<1>))(socket&&, yield_context)>
                 *
                 * But solution below works, and why? (https://stackoverflow.com/questions/4871273/passing-rvalues-through-stdbind)
                 * auto _fdebug = std::bind(&agent::do_session, this, std::bind(static_cast<tcp::socket&&(&)(tcp::socket&)>(std::move<tcp::socket&>), std::move(conn)), std::placeholders::_1);
                 * _fdebug(yield);
                 *
                 * asio::spawn(io_context, std::bind(&agent::do_session, this, std::move(conn), std::placeholders::_1));
                 */
                asio::spawn(io_context, std::bind(&agent::do_session, this, std::bind(static_cast<tcp::socket&&(&)(tcp::socket&)>(std::move<tcp::socket&>), std::move(conn)), std::placeholders::_1));
            }
        }
    }

    void agent::do_session(tcp::socket &&conn, asio::yield_context yield) {
        boost::system::error_code ec;
        boost::beast::flat_buffer buffer;

        while (true) {
            http::request<http::string_body> req;

            http::async_read(conn, buffer, req, yield[ec]);
            if (ec == http::error::end_of_stream)
                break;
            if (ec) ON_BOOST_ERROR(ec);

            auto response = handle_request(std::move(req), yield);

            http::async_write(conn, response, yield[ec]);

            if (!response.keep_alive())
                break;
        }

        conn.shutdown(tcp::socket::shutdown_send, ec);
        if (ec) ON_BOOST_ERROR(ec);
    }

    http::response<http::string_body> agent::handle_request(http::request<http::string_body> &&req, asio::yield_context &yield) {
        static const auto resp_400 = []{
            http::response<http::string_body> res{http::status::bad_request, 11};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(false);
            res.body() = "bad request";
            res.prepare_payload();
            return std::move(res);
        }();
        // Only serve POST request. Return 400 if not GET.
        if (req.method() != http::verb::post) {
            rlog.debug("Request is not post. Giving 400.");
            return resp_400;
        }

        provider_info *provider = selector.query_once();
        if(provider == nullptr) {
            rlog.error("All server latency over 100us... Dropping request...");
            return resp_400; // TODO: must use 502
        }
        req.set(http::field::user_agent, "rHttp");
        req.set(http::field::host, provider->get_host());

        auto time_L = std::chrono::high_resolution_clock::now();
        auto res = provider->async_request(req, yield);
        auto time_R = std::chrono::high_resolution_clock::now();
        auto _latency = std::chrono::duration_cast<std::chrono::microseconds>(time_R - time_L).count();
        rlog.debug("async_req latency = {}"_format(_latency));

        res.set(http::field::server, "rHttp");
        if (res.result() == http::status::internal_server_error) {
            res.keep_alive(false);
            rlog.error("Warning: interal server error detected. refuse to keep_alive.");
        } else {
            res.keep_alive(req.keep_alive());
        }
        return std::move(res);
    }
}
