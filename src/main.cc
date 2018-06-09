//
// Created by recolic on 18-5-16.
//

#include <rlib/opt.hpp>
#include <rlib/stdio.hpp>
#include <rlib/log.hpp>
#include <rlib/macro.hpp>

#include <producer_agent.hpp>
#include <consumer_agent.hpp>

rlib::logger rlog(std::cout);

using rlib::print;
using rlib::println;
using namespace rlib::literals;

int main(int argc, char **argv) {
    auto help_and_exit = [&argv]{
        print(R"RALI(
Ali-middleware challenge 2018 {}
CopyRight (C) 2018 - 2018
    Recolic Keghart <root@recolic.net>
    Yue Pan <zxc479773533@gmail.com>
    Alisa Wu <wuminyan0607@gmail.com>

Usage: {} <consumer/producer-*> [Args ...]

Args:
->>> consumer
<<<<<<< HEAD

->>> producer
[Required] --consumer      -c  The consumer-agent address.
[Required] --consumer-port -cp The port of consumer-agent.
[Required] --listen        -l  Address where producer-agent listens.
[Required] --listen-port   -lp  Port where producer-agent listens.
[Required] --etcd              Address of etcd service.
[Required] --etcd-port         Address of etcd port.
=======
[Required] --listen       -l  Address where consumer-agent listens.
[Required] --listen-port  -p  Port where consumer-agent listens.
[Required] --etcd             Address of etcd service.
[Required] --etcd-port        Address of etcd port.

->>> producer
>>>>>>> master
)RALI"_format(RLIB_MACRO_TO_CSTR(AGENT_VERSION), argv[0]));
        exit(1);
    };

    rlog.set_log_level(rlib::log_level_t::DEBUG);
    if(argc == 1)
        help_and_exit();

    rlib::opt_parser opt(argc, argv);
    if (opt.getBoolArg("--help", "-h"))
        help_and_exit();

    auto whoami = opt.getCommand();

    if (whoami.substr(0, 8) == "producer") {
        auto consumer_addr = opt.getValueArg("--consumer", "-c");
        auto consumer_port = opt.getValueArg("--consumer-port", "-cp").as<uint16_t>();

        auto listen_addr = opt.getValueArg("--listen", "-l");
        auto listen_port = opt.getValueArg("--listen-port", "-p").as<uint16_t>();

        auto etcd_addr = opt.getValueArg("--etcd");
        auto etcd_port = opt.getValueArg("--etcd-port").as<uint16_t>();

        producer::agent agent("{}:{}"_format(etcd_addr, etcd_port), consumer_addr, consumer_port);
        rlog.debug("Agent initialize done.");
        agent.listen(listen_addr, listen_port);
    }
    else if(whoami == "consumer") {
        auto listen_addr = opt.getValueArg("--listen", "-l");
        auto listen_port = opt.getValueArg("--listen-port", "-p").as<uint16_t>();

        auto etcd_addr = opt.getValueArg("--etcd");
        auto etcd_port = opt.getValueArg("--etcd-port").as<uint16_t>();

        //auto debug_server_addr = opt.getValueArg("--debug-server-addr", false);
        //auto debug_server_port = opt.getValueArg("--debug-server-port", false, "0").as<uint16_t>();

        consumer::agent agent("{}:{}"_format(etcd_addr, etcd_port));
        rlog.debug("Agent initialize done.");
        agent.listen(listen_addr, listen_port);
    }
    else {
        rlog.fatal("Role must be consumer or producer-*, rather than `{}`."_format(whoami));
        help_and_exit();
    }

}
