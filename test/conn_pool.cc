//
// Created by recolic on 18-6-3.
//

#include <conn_pool.hpp>
#include <rlib/log.hpp>

rlib::logger rlog(std::cout);

class something {
public:
    something(boost::asio::io_context &ioc, const std::string &s, uint16_t n)
            : pool(ioc, s, n) {}

    consumer::conn_pool pool;
};

int main() {
    rlog.set_log_level(rlib::log_level_t::DEBUG);
    boost::asio::io_context ioc;
    std::string s = "www.baidu.com";
    something a(ioc, s, 80);
    //consumer::conn_pool pool(ioc, s, 80);
    auto res = a.pool.borrow_one();
}