//
// Created by recolic on 18-6-18.
//

#include <rlib/stdio.hpp>
#include <rlib/opt.hpp>
#include <logger.hpp>
#include <boost_asio_quick_connect.hpp>
#include <boost/asio.hpp>

using namespace boost::asio;
using boost::asio::ip::tcp;
using namespace std::chrono_literals;
rlib::logger rlog(std::cout);

int main(int argc, char **argv) {
    rlib::opt_parser opt(argc, argv);
    io_context ioc;
    rlog.set_log_level(rlib::log_level_t::DEBUG);

    std::string data = "gjww";
    char buf[1024];
    data += std::string(1020, 'w');


    if (opt.getCommand() == "consumer") {
        auto sock = quick_connect(ioc, opt.getValueArg("--server"), 25427);
        rlog.info("tcpping: connected.");
        while (true) {
            std::this_thread::sleep_for(2s);
            RDEBUG_CURR_TIME_VAR(timeL);
            write(sock, buffer(data));
            sock.read_some(buffer(buf, 1024));
            RDEBUG_CURR_TIME_VAR(timeR);
            RDEBUG_LOG_TIME_DIFF(timeR, timeL, "tcpping latency");
        }
    } else {
        rlog.info("tcpping: server launched.");
        boost::system::error_code ec;
        tcp::endpoint endpoint(ip::make_address("0.0.0.0"), 25427);
        tcp::acceptor acceptor(ioc);
        acceptor.open(endpoint.protocol(), ec);
        if (ec) ON_BOOST_FATAL(ec);
        acceptor.set_option(socket_base::reuse_address(true));
        if (ec) ON_BOOST_FATAL(ec);
        acceptor.bind(endpoint, ec);
        if (ec) ON_BOOST_FATAL(ec);
        acceptor.listen(socket_base::max_listen_connections, ec);
        if (ec) ON_BOOST_FATAL(ec);

        while (true) {
            tcp::socket conn(ioc);
            acceptor.accept(conn);
            rlog.info("Accepted.");
            try {
                while (true) {
                    conn.read_some(buffer(buf, 1024));
                    std::this_thread::sleep_for(50ms);
                    write(conn, buffer(data));
                }
            } catch (...) {}
        }
    }
    ioc.run();

}
