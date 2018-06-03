#ifndef _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP
#define _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP 1

#include <boost/asio.hpp>

#include <rlib/log.hpp>

using rlib::literals::operator ""_format;
extern rlib::logger rlog;

namespace boost {
    namespace asio {
        inline ip::tcp::socket quick_connect(io_context &io_context, const std::string &addr, uint16_t port) {
            ip::tcp::socket sock(io_context);
            ip::tcp::resolver resolver(io_context);

            rlog.verbose_info("Connecting to {}:{}"_format(addr, port));
            connect(sock, resolver.resolve(ip::tcp::resolver::query(addr, std::to_string(port))));
            rlog.verbose_info("Connected.");
            return sock;
        }
    }
}

#endif //_ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP
