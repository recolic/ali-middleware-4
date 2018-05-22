//
// Created by recolic on 18-5-16.
//

#include <rlib/opt.hpp>
#include <rlib/stdio.hpp>
#include <rlib/log.hpp>
#include <rlib/macro.hpp>

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
[Required] --listen       -l  Address where consumer-agent listens.
[Required] --listen-port  -p  Port where consumer-agent listens.
[Required] --etcd             Address of etcd service.
[Required] --etcd-port        Address of etcd port.

->>> producer
)RALI"_format(RLIB_MACRO_TO_CSTR(AGENT_VERSION), argv[0]));
        exit(1);
    };

    if(argc == 1)
        help_and_exit();

    rlib::opt_parser opt(argc, argv);
    if (opt.getBoolArg("--help", "-h"))
        help_and_exit();

    auto whoami = opt.getCommand();

    if (whoami.substr(0, 8) == "producer") {
        // TODO: parse and call

    }
    else if(whoami == "consumer") {
        auto listen_addr = opt.getValueArg("--listen", "-l");
        auto listen_port = opt.getValueArg("--listen-port", "-p").as<uint16_t>();

        auto etcd_addr = opt.getValueArg("--etcd");
        auto etcd_port = opt.getValueArg("--etcd-port").as<uint16_t>();

        //auto debug_server_addr = opt.getValueArg("--debug-server-addr", false);
        //auto debug_server_port = opt.getValueArg("--debug-server-port", false, "0").as<uint16_t>();

        consumer::agent agent("{}:{}"_format(etcd_addr, etcd_port));
        agent.listen(listen_addr, listen_port);
    }
    else {
        help_and_exit();
    }


}
