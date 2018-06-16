//
// Created by recolic on 18-6-3.
//

#ifndef ALI_MIDDLEWARE_AGENT_CONN_POOL_HPP
#define ALI_MIDDLEWARE_AGENT_CONN_POOL_HPP

#include <pool.hpp>
#include <boost/asio.hpp>
#include <boost_asio_quick_connect.hpp>

namespace consumer {
    // TODO: if necessary, implement pooled_coro_conn. (boost::asio::async_quick_connect is available)
    class pooled_conn {
    public:
        pooled_conn(boost::asio::io_context &ioc, std::string addr, uint16_t port)
                : conn(boost::asio::quick_connect(ioc, addr, port)) {}

        pooled_conn(pooled_conn &&another) : conn(std::move(another.conn)) {}

        boost::asio::ip::tcp::socket &get() {
            return conn;
        }

        ~pooled_conn() {
            boost::system::error_code ec;
            conn.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        }

    private:
        boost::asio::ip::tcp::socket conn;
    };

    using conn_pool = rlib::fixed_object_pool<pooled_conn, AGENT_CONN_POOL_SIZE, boost::asio::io_context &, std::string, uint16_t>;
    using conn_pool_coro = rlib::fixed_object_pool_coro<pooled_conn, AGENT_CONN_POOL_SIZE, boost::asio::io_context &, std::string, uint16_t>;

}

namespace dubbo {
    using conn_pool_coro = rlib::fixed_object_pool_coro<consumer::pooled_conn, DUBBO_CLI_CONN_POOL_SIZE, boost::asio::io_context &, std::string, uint16_t>;
}


#endif //ALI_MIDDLEWARE_AGENT_CONN_POOL_HPP
