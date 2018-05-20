//
// Created by recolic on 18-5-18.
//

#include "producer_selector.hpp"

consumer::producer_info::producer_info(boost::asio::io_context &ioContext, const std::string &addr, uint16_t port)
        : io_context(ioContext), conn(io_context) {
    // TODO: Make connection to server(addr:port) with socket conn.
}
