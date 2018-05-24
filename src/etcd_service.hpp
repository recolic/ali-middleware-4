//
// Created by recolic on 18-5-25.
//

#ifndef ALI_MIDDLEWARE_AGENT_ETCD_SERVICE_HPP
#define ALI_MIDDLEWARE_AGENT_ETCD_SERVICE_HPP

#include <rlib/class_decorator.hpp>
#include <string>
#include <boost/asio.hpp>
#include <unordered_map>

/*
 * This class is for current usage. It implements limited functions, to support current naive agent.
 */
class etcd_service : rlib::noncopyable {
public:
    using key_type = std::string;
    using value_type = std::string;
    using container_type = std::vector<std::pair<key_type, value_type>>;

    etcd_service() = delete;

    etcd_service(const std::string &etcd_addr_and_port) : conn(io_context) {
        connect(etcd_addr_and_port);
    }

    void connect(const std::string &etcd_addr_and_port);

    void append(const key_type &key, const value_type &value);
    const container_type &get_list(const key_type &key);

private:
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket conn;

    std::vector<std::pair<key_type, value_type>> cache;


#endif //ALI_MIDDLEWARE_AGENT_ETCD_SERVICE_HPP
