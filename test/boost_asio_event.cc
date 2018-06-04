//
// Created by recolic on 18-6-4.
// Test for boost_asio_event.hpp

#include <boost_asio_event.hpp>
#include <rlib/stdio.hpp>
#include <thread>
#include <boost/asio/spawn.hpp>

using namespace rlib::literals;

using boost::asio::event;

void test_proc(boost::asio::io_context &ioc, boost::asio::yield_context yield) {
    event ev(ioc);

    auto waiting_handler = [](const boost::system::error_code &ec) {
        rlib::println("wait done. err_code is {}."_format(ec.message()));
    };

    ev.async_wait(waiting_handler);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ev.notify_all();

    rlib::println("test2");
    boost::asio::deadline_timer tm(ioc);
    tm.async_wait(waiting_handler);
    tm.expires_at(boost::posix_time::pos_infin);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    tm.cancel_one();
}

int main() {
    boost::asio::io_context ioc;
    boost::asio::spawn(ioc, std::bind(test_proc, std::ref(ioc), std::placeholders::_1));

    ioc.run();
}
