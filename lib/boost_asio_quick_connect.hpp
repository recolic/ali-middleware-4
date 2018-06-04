#ifndef _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP
#define _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP 1

#include <boost/asio.hpp>

#include <boost/asio/spawn.hpp>
#include <logger.hpp>

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

        inline ip::tcp::socket async_quick_connect(io_context &io_context, const std::string &addr, uint16_t port,
                                                   boost::asio::yield_context &yield) {
            ip::tcp::socket sock(io_context);
            ip::tcp::resolver resolver(io_context);

            boost::system::error_code ec;
            auto res = resolver.async_resolve(ip::tcp::resolver::query(addr, std::to_string(port)), yield[ec]);
            if (ec) RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
            async_connect(sock, res, yield[ec]);
            if (ec) RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
            return sock;
        }
    }
}

#endif //_ALI_MIDDLEWARE_AGENT_BOOST_ASIO_QUICK_CONNECT_HPP
