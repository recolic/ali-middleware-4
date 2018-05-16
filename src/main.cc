//
// Created by recolic on 18-5-16.
//

#include <rlib/opt.hpp>
#include <rlib/stdio.hpp>

using rlib::println;
using namespace rlib::literals;

int main(int argc, char **argv) {
    auto help_and_exit = [&argv]{
        println("Usage: {} <producer/consumer> [args ...]"_format(argv[0]));
        exit(1);
    };

    if(argc == 1)
        help_and_exit();

    std::string whoami = argv[1];
    rlib::opt_parser opt(argc, argv);
    if(whoami == "producer") {
        // TODO: parse and call

    }
    else if(whoami == "consumer") {

    }
    else {
        help_and_exit();
    }


}
