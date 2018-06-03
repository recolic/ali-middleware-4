//
// Created by recolic on 18-6-3.
//

#ifndef ALI_MIDDLEWARE_AGENT_CONN_POOL_HPP
#define ALI_MIDDLEWARE_AGENT_CONN_POOL_HPP

#include <pool.hpp>
#include <boost/asio.hpp>
#include <boost_asio_quick_connect.hpp>

namespace consumer {
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

    using conn_pool = rlib::fixed_object_pool<pooled_conn, 256, boost::asio::io_context &, std::string, uint16_t>;
}


#endif //ALI_MIDDLEWARE_AGENT_CONN_POOL_HPP
