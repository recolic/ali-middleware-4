//
// Created by recolic on 18-5-16.
//

#include <rlib/opt.hpp>
#include <rlib/stdio.hpp>
#include <rlib/log.hpp>

rlib::logger rlog(std::cout);

using rlib::println;
using namespace rlib::literals;

int main(int argc, char **argv) {
    auto help_and_exit = [&argv]{
        println("Usage: {} <producer-*/consumer> [args ...]"_format(argv[0]));
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

        auto debug_server_addr = opt.getValueArg("--debug-server-addr", false);
        auto debug_server_port = opt.getValueArg("--debug-server-port", false, "0").as<uint16_t>();
    }
    else {
        help_and_exit();
    }


}
