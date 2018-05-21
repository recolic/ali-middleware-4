#ifndef _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP
#define _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP 1

#include <boost/asio.hpp>

namespace boost {
    namespace asio {
        inline boost::asio::ip::tcp::socket
        quick_connect(boost::asio::io_context &io_context, const std::string &addr, uint16_t port) {
            boost::asio::ip::tcp::socket sock(io_context);
            boost::asio::ip::tcp::resolver resolver(io_context);

            boost::asio::connect(sock,
                                 resolver.resolve(boost::asio::ip::tcp::resolver::query(addr, std::to_string(port))));
            return sock;
        }
    }
}

#endif //_ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP
