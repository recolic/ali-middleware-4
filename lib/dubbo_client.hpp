//
// Created by recolic on 18-5-16.
//

#ifndef ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP
#define ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP

#include <string>
#include <list>
#include <rlib/sys/os.hpp>
#include <rlib/sys/sio.hpp>
#include <rlib/scope_guard.hpp>

#include <json_serializer.hpp>
#include <boost/asio.hpp>
#include <boost_asio_quick_connect.hpp>

class dubbo_client
{
public:
    dubbo_client() = delete;
    dubbo_client(const std::string &server_addr, uint16_t server_port)
            : server_addr(server_addr), server_port(server_port) {}

    enum class status_t {OK = 20, CLIENT_TIMEOUT = 30, SERVER_TIMEOUT = 31, BAD_REQUREST = 40,
            BAD_RESPONSE = 50, SERVICE_NOT_FOUND = 60, SERVICE_ERROR = 70, SERVER_ERROR = 80,
                    CLIENT_ERROR = 90, SERVER_THREADPOOL_EXHAUSTED_ERROR = 100};

    struct request_result_t {
        status_t status;
        kv_serializer::kv_list_t payload;
    };

    request_result_t request(const kv_serializer::kv_list_t &parameter) {
        return std::move(request(json_serializer().serialize(parameter)));
    }
    request_result_t request(const std::string &payload) {
        dubbo_header header;
        header.is_request = 1;
        header.need_return = 1;
        header.is_event = 0;
        header.serialization_id = 6;
        header.status = 0;
        header.request_id = 0x19990713;
        header.data_length = (uint32_t)payload.size();

        // TODO: reuse connection.
        auto sockServer = boost::asio::quick_connect(io_context, server_addr, server_port);

        boost::asio::write(sockServer, boost::asio::buffer(&header, sizeof(header)));
        boost::asio::write(sockServer, boost::asio::buffer(payload));

        boost::asio::read(sockServer, boost::asio::buffer(&header, sizeof(header)));
        if(header.data_length > 1024 * 1024 * 1024)
            throw std::runtime_error("Dubbo server says the payload length is >1GiB. It's dangerous so I rejected.");
        std::string payload_buffer;
        payload_buffer.reserve(header.data_length);
        boost::asio::read(sockServer, boost::asio::buffer(payload_buffer));

        request_result_t result;
        result.status = (status_t)header.status;
        result.payload = json_serializer().deserialize(payload_buffer);
        return std::move(result);
    }

private:
    std::string server_addr;
    uint16_t server_port;

    boost::asio::io_context io_context;

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
