//
// Created by recolic on 18-5-17.
//

#include <consumer_agent.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/spawn.hpp>

#include <functional>

using string_view = boost::beast::string_view;
namespace http = boost::beast::http;

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;

using namespace rlib::literals;

namespace consumer {
#define ON_BOOST_FATAL(ec) do { RBOOST_LOG_EC(ec, rlib::log_level_t::FATAL); \
        throw std::runtime_error(ec.message()); } while(false)
#define ON_BOOST_ERROR(ec) do { RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR); \
        return; } while(false)
#define RBOOST_LOG_EC(ec, level) rlog.log("boost error at {}:{}, {}"_format(__FILE__, __LINE__, ec.message()), level)

    [[noreturn]] void agent::listen(const std::string &listen_addr, uint16_t listen_port) {
        // TODO: Launch http server and forward requests.
        // TODO: you have to reuse connection to every server.
        tcp::endpoint endpoint(ip::make_address(listen_addr), listen_port);

        static_assert(std::is_same<decltype(std::bind(&agent::do_listen, this, endpoint, std::placeholders::_1)(
                *(asio::yield_context *) nullptr)),
                void>::value, "Error on deducting type for asio::spawn arg2.");
        rlog.info("Launching http server (consumer_agent) at {}:{}"_format(listen_addr, listen_port));
        asio::spawn(io_context, std::bind(&agent::do_listen, this, std::move(endpoint), std::placeholders::_1));
    }

    void agent::do_listen(tcp::endpoint endpoint, asio::yield_context yield) {
        boost::system::error_code ec;

        tcp::acceptor acceptor(io_context);
        acceptor.open(endpoint.protocol(), ec);
        if (ec) ON_BOOST_FATAL(ec);

        acceptor.set_option(boost::asio::socket_base::reuse_address(true));
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
                //std::_Bind_helper<0, void (consumer::agent::*)(tcp::socket, asio::yield_context), consumer::agent *, tcp::socket, std::_Placeholder<1> &>::type _fdebug = std::bind(&agent::do_session, this, std::move(conn), std::placeholders::_1);
                //_fdebug(yield);
                /*
                 * consumer_agent.cc:64:17: error: no matching function for call to object of type 'std::_Bind_helper<0, void (agent::*)(tcp::socket, asio::yield_context),
      consumer::agent *, tcp::socket, std::_Placeholder<1> &>::type' (aka '_Bind<void (consumer::agent::*(consumer::agent *,
      boost::asio::basic_stream_socket<boost::asio::ip::tcp>, std::_Placeholder<1>))(boost::asio::basic_stream_socket<boost::asio::ip::tcp>,
      boost::asio::basic_yield_context<boost::asio::executor_binder<void (*)(), boost::asio::executor> >)>')
                 * */
                // TODO: (Compilation error) What's the matter???????????????????????
                asio::spawn(io_context, std::bind(&agent::do_session, this, std::move(conn), std::placeholders::_1));
            }
        }
    }

    void agent::do_session(tcp::socket conn, asio::yield_context yield) {
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

    http::response agent::handle_request(http::request<http::string_body> &&req, asio::yield_context &yield) {
        std::string res_payload = "bad request";

        // Only serve GET request. Return 400 if not GET.
        if (req.method() != http::verb::get) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, "rHttp");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = res_payload;
            res.prepare_payload();
            return std::move(res);
        }

        producer_info &producer = selector.query_once();

    }

#undef ON_BOOST_FATAL
#undef ON_BOOST_ERROR
#undef RBOOST_LOG_EC
}
