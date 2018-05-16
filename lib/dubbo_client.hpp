//
// Created by recolic on 18-5-16.
//

#ifndef ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP
#define ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP

#include <string>
#include <list>
#include <rlib/sys/os.hpp>

#include "json_serializer.hpp"

class dubbo_client
{
public:
    dubbo_client() = delete;
    dubbo_client(const std::string &server_addr, uint16_t server_port)
            : server_addr(server_addr), server_port(server_port) {}

    enum class status_t {OK = 20, CLIENT_TIMEOUT = 30, SERVER_TIMEOUT = 31, BAD_REQUREST = 40,
            BAD_RESPONSE = 50, SERVICE_NOT_FOUND = 60, SERVICE_ERROR = 70, SERVER_ERROR = 80,
                    CLIENT_ERROR = 90, SERVER_THREADPOOL_EXHAUSTED_ERROR = 100};

    status_t request(const kv_serializer::kv_list_t &parameter) {
        return request(json_serializer::serialize(parameter));
    }
    status_t request(const std::string &payload) {

    }

private:
    std::string server_addr;
    uint16_t server_port;

    struct dubbo_header {
        uint16_t magic = 0xdabb;
        unsigned is_request : 1;
        unsigned need_return : 1;
        unsigned is_event : 1;
#if RLIB_CXX_STD >= 2020
        unsigned serialization_id : 5 = 6;
#else
        unsigned serialization_id : 5;
#endif
        uint8_t status;
        uint64_t request_id;
        uint32_t data_length;
    } __attribute__((packed));
};


#endif //ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP
