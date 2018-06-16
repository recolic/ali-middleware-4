//
// Created by recolic on 18-5-16.
//

#ifndef ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP
#define ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP

#include <logger.hpp>
#include <string>
#include <list>
#include <rlib/sys/os.hpp>
#include <rlib/sys/sio.hpp>

#include <json_serializer.hpp>
#include <boost/asio.hpp>
#include <boost_asio_quick_connect.hpp>

#include <rlib/scope_guard.hpp>
#define BOOST_ENDIAN_DEPRECATED_NAMES
#include <boost/endian/endian.hpp>

class dubbo_client
{
public:
    dubbo_client() = delete;

    dubbo_client(boost::asio::io_context &ioc, const std::string &server_addr, uint16_t server_port)
            : io_context(ioc), server_addr(server_addr), server_port(server_port), curr_request_id(0x1998427) {}

    enum class status_t {OK = 20, CLIENT_TIMEOUT = 30, SERVER_TIMEOUT = 31, BAD_REQUREST = 40,
            BAD_RESPONSE = 50, SERVICE_NOT_FOUND = 60, SERVICE_ERROR = 70, SERVER_ERROR = 80,
                    CLIENT_ERROR = 90, SERVER_THREADPOOL_EXHAUSTED_ERROR = 100};

    struct request_result_t {
        status_t status;
        kv_serializer::kv_list_t payload;
    };

    request_result_t sync_request(const std::string &service_name, const std::string &method_name, const std::string &method_arg_type, const std::string &method_arg) {
        // You must NEVER edit this string below without carefully consideration!
        const std::string payload = R"RALI("2.6.1"
"{}"
null
"{}"
"{}"
"{}"
{"path":"{}"}
)RALI"_format(service_name, method_name, method_arg_type, method_arg, service_name);
        // You must NEVER edit this string above without carefully consideration!

        dubbo_header header;
        header.is_request = 1;
        header.need_return = 1;
        header.is_event = 0;
        header.serialization_id = 6;
        header.status = 0;
        header.request_id = curr_request_id;
        header.data_length = boost::endian::native_to_big((uint32_t)payload.size());

        rlog.debug("Payload size {} is ready."_format(payload.size()));
        curr_request_id += 1;

        // TODO: reuse connection.
        auto sockServer = boost::asio::quick_connect(io_context, server_addr, server_port);

        boost::asio::write(sockServer, boost::asio::buffer(&header, sizeof(header)));
        boost::asio::write(sockServer, boost::asio::buffer(payload));
        rlog.debug("dubbo message sent.");
        boost::asio::read(sockServer, boost::asio::buffer(&header, sizeof(header)));
        header.data_length = boost::endian::big_to_native(header.data_length);

        rlog.debug("dubbo return info: is_req={}, need_ret={}, is_event={}, serialize_id={}, status={}"_format(header.is_request, header.need_return, header.is_event, header.serialization_id, header.status));
        rlog.debug("    req_id={}, data_len={}"_format(header.request_id, header.data_length));

        if(header.data_length > 1024 * 1024 * 1024)
            throw std::runtime_error("Dubbo server says the payload length is >1GiB. It's dangerous so I rejected.");
        std::string payload_buffer(header.data_length, 'E');
        //char *debug_vct = new char[header.data_length]{};
        boost::asio::read(sockServer, boost::asio::buffer(const_cast<char *>(const_cast<std::string &>(payload_buffer).data()), header.data_length));
        //auto sz = boost::asio::read(sockServer, boost::asio::buffer(debug_vct, header.data_length));
        //rlog.debug("readn ret = {}"_format(sz));
        //std::string payload_buffer(debug_vct, header.data_length);
        rlog.debug("read payload = `{}`"_format(payload_buffer));


        if(header.magic != 0xbbda)
            throw std::runtime_error("dubbo_client.hpp:Dubbo returns wrong magic.");

        request_result_t result;
        result.status = (status_t)header.status;
        result.payload = json_serializer().deserialize(payload_buffer);
        return std::move(result);
    }


    request_result_t async_request(const kv_serializer::kv_list_t &parameter, boost::asio::yield_context &yield) {
        return std::move(async_request(json_serializer().serialize(parameter), yield));
    }

    request_result_t async_request(const std::string &payload, boost::asio::yield_context yield) {
        dubbo_header header;
        header.is_request = 1;
        header.need_return = 1;
        header.is_event = 0;
        header.serialization_id = 6;
        header.status = 0;
        header.request_id = curr_request_id;
        header.data_length = (uint32_t)payload.size();

        curr_request_id += 1;

        // TODO: reuse connection.
        auto sockServer = boost::asio::async_quick_connect(io_context, server_addr, server_port, yield);

        boost::system::error_code ec;
        boost::asio::async_write(sockServer, boost::asio::buffer(&header, sizeof(header)), yield[ec]);
        if (ec) ON_BOOST_FATAL(ec);
        boost::asio::async_write(sockServer, boost::asio::buffer(payload), yield[ec]);
        if (ec) ON_BOOST_FATAL(ec);
        boost::asio::async_read(sockServer, boost::asio::buffer(&header, sizeof(header)), yield[ec]);
        if (ec) ON_BOOST_FATAL(ec);
        if(header.data_length > 1024 * 1024 * 1024)
            throw std::runtime_error("Dubbo server says the payload length is >1GiB. It's dangerous so I rejected.");
        std::string payload_buffer;
        payload_buffer.reserve(header.data_length);
        boost::asio::async_read(sockServer, boost::asio::buffer(payload_buffer), yield[ec]);
        if (ec) ON_BOOST_FATAL(ec);

        rlog.debug("dubbo return info: is_req={}, need_ret={}, is_event={}, serialize_id={}, status={}"_format(header.is_request, header.need_return, header.is_event, header.serialization_id, header.status));
        rlog.debug("    req_id={}, data_len={}, data=`{}`"_format(header.request_id, header.data_length, payload_buffer));

        if(header.magic != 0xbbda)
            throw std::runtime_error("dubbo_client.hpp:Dubbo returns wrong magic.");

        request_result_t result;
        result.status = (status_t)header.status;
        result.payload = json_serializer().deserialize(payload_buffer);
        return std::move(result);
    }

private:
    std::string server_addr;
    uint16_t server_port;

    std::atomic_uint64_t curr_request_id;
    boost::asio::io_context &io_context;

    struct dubbo_header {
        uint16_t magic = 0xbbda;
#if defined(RDUBBO_OBEY_ALIDOC)
        unsigned is_request : 1;
        unsigned need_return : 1;
        unsigned is_event : 1;
#define RDUBBO_DOC_DEFINED
#endif
#if RLIB_CXX_STD >= 2020
        unsigned serialization_id : 5 = 6;
#else
        unsigned serialization_id : 5;
#endif
#if defined(RDUBBO_OBEY_APACHEDOC)
        unsigned is_event : 1;
        unsigned need_return : 1;
        unsigned is_request : 1;
#define RDUBBO_DOC_DEFINED
#endif
        uint8_t status;
        uint64_t request_id;
        uint32_t data_length;
    } __attribute__((packed));
};

#ifndef RDUBBO_DOC_DEFINED
#error You must define either RDUBBO_OBEY_ALIDOC or RDUBBO_OBEY_APACHEDOC.
#endif

#endif //ALI_MIDDLEWARE_AGENT_DUBBO_CLIENT_HPP
