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

Usage: {} <consumer/producer-*> [Args ...]

Args:
->>> consumer
[Required] --listen       -l  Address where the agent listens.
[Required] --listen-port  -p  Port where the agent listens.
[Required] --etcd             Address of etcd service.
[Required] --etcd-port        Address of etcd port.
           --log              (info/debug/fatal) set log level, default=info.

->>> producer
[Required] --listen        -l  Address where the agent listens.
[Required] --listen-port   -p  Port where the agent listens.
[Required] --producer          Address of producer (dubbo server).
[Required] --producer-port     Port of producer.
[Required] --etcd              Address of etcd service.
[Required] --etcd-port         Address of etcd port.
)RALI"_format(RLIB_MACRO_TO_CSTR(AGENT_VERSION), argv[0]));
        exit(1);
    };

    if(argc == 1)
        help_and_exit();

    rlib::opt_parser opt(argc, argv);
    if (opt.getBoolArg("--help", "-h"))
        help_and_exit();

    auto whoami = opt.getCommand();
    auto log_level = opt.getValueArg("--log", false);
    if(log_level == "fatal") rlog.set_log_level(rlib::log_level_t::FATAL);
    else if(log_level == "info") rlog.set_log_level(rlib::log_level_t::INFO);
    else if(log_level == "debug") rlog.set_log_level(rlib::log_level_t::DEBUG);

    if (whoami.substr(0, 8) == "producer") {

        auto listen_addr = opt.getValueArg("--listen", "-l");
        auto listen_port = opt.getValueArg("--listen-port", "-p").as<uint16_t>();

        auto producer_addr = opt.getValueArg("--producer");
        auto producer_port = opt.getValueArg("--producer-port").as<uint16_t>();

        auto etcd_addr = opt.getValueArg("--etcd");
        auto etcd_port = opt.getValueArg("--etcd-port").as<uint16_t>();

        producer::agent agent("{}:{}"_format(etcd_addr, etcd_port), producer_addr, producer_port);
        rlog.info("'{}' is listening {}:{} as producer, with etcd server set to {}:{}, producer set to {}:{}."_format(whoami, listen_addr, listen_port, etcd_addr, etcd_port, producer_addr, producer_port));
        agent.listen(listen_addr, listen_port);
    }
    else if(whoami == "consumer") {
        auto listen_addr = opt.getValueArg("--listen", "-l");
        auto listen_port = opt.getValueArg("--listen-port", "-p").as<uint16_t>();

        auto etcd_addr = opt.getValueArg("--etcd");
        auto etcd_port = opt.getValueArg("--etcd-port").as<uint16_t>();

        consumer::agent agent("{}:{}"_format(etcd_addr, etcd_port));
        rlog.info("'{}' is listening {}:{} as consumer, with etcd server set to {}:{}."_format(whoami, listen_addr, listen_port, etcd_addr, etcd_port));
        agent.listen(listen_addr, listen_port);
    }
    else {
        rlog.fatal("Role must be consumer or producer-*, rather than `{}`."_format(whoami));
        help_and_exit();
    }

}
