//
// Created by recolic on 18-5-16.
//

#include <rlib/opt.hpp>
#include <rlib/stdio.hpp>
#include <rlib/log.hpp>
#include <rlib/macro.hpp>

#include <provider_agent.hpp>
#include <consumer_agent.hpp>

rlib::logger rlog(std::cout);

using rlib::print;
using rlib::println;
using namespace rlib::literals;

int main(int argc, char **argv) {
    auto help_and_exit = [&argv](int _stat){
        print(R"RALI(
Ali-middleware challenge 2018 {}
    > GIT {}
CopyRight (C) 2018 - 2018
    Recolic Keghart <root@recolic.net>
    Yue Pan <zxc479773533@gmail.com>

Usage: {} <consumer/provider-*> [Args ...]

Args:
->>> common
[Required] --listen       -l  Address where the agent listens.
[Required] --listen-port  -p  Port where the agent listens.
[Required] --etcd             Address of etcd service.
[Required] --etcd-port        Address of etcd port.
           --log              (info/debug/fatal) set log level, default=info.

->>> consumer-agent


->>> provider-agent
[Required] --provider          Address of provider (dubbo server).
[Required] --provider-port     Port of provider.
)RALI"_format(RLIB_MACRO_TO_CSTR(AGENT_VERSION), RLIB_MACRO_TO_CSTR(GIT_COMMIT_NUM), argv[0]));
        exit(_stat);
    };

    if(argc == 1)
        help_and_exit(1);

    rlib::opt_parser opt(argc, argv);
    if (opt.getBoolArg("--help", "-h"))
        help_and_exit(0);

    auto whoami = opt.getCommand();
    auto log_level = opt.getValueArg("--log", false);
    if(log_level == "fatal") rlog.set_log_level(rlib::log_level_t::FATAL);
    else if(log_level == "info") rlog.set_log_level(rlib::log_level_t::INFO);
    else if(log_level == "debug") rlog.set_log_level(rlib::log_level_t::DEBUG);

    if (whoami.substr(0, 8) == "provider") {

        auto listen_addr = opt.getValueArg("--listen", "-l");
        auto listen_port = opt.getValueArg("--listen-port", "-p").as<uint16_t>();

        auto provider_addr = opt.getValueArg("--provider");
        auto provider_port = opt.getValueArg("--provider-port").as<uint16_t>();

        auto etcd_addr = opt.getValueArg("--etcd");
        auto etcd_port = opt.getValueArg("--etcd-port").as<uint16_t>();

        provider::agent agent("{}:{}"_format(etcd_addr, etcd_port), provider_addr, provider_port);
        rlog.info("'{}' is listening {}:{} as provider, with etcd server set to {}:{}, provider set to {}:{}."_format(whoami, listen_addr, listen_port, etcd_addr, etcd_port, provider_addr, provider_port));
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
        rlog.fatal("Role must be consumer or provider-*, rather than `{}`."_format(whoami));
        help_and_exit(1);
    }

}
