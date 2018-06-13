//
// Created by recolic on 18-5-25.
//

#ifndef ALI_MIDDLEWARE_AGENT_ETCD_SERVICE_HPP
#define ALI_MIDDLEWARE_AGENT_ETCD_SERVICE_HPP

#include <etcd/Client.hpp>
#include "logger.hpp"

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

    etcd_service(const std::string &etcd_addr_and_port) : cli(etcd_addr_and_port) {
        rlog.info("Connected to {}."_format(etcd_addr_and_port));
    }

    void append(const key_type &key, const value_type &value);
    std::vector<rlib::string> get_list(const key_type &key);

private:
    value_type sync_get(const key_type &key) {
        auto resp = cli.get(key).get();
        if(!resp.is_ok())
            throw std::runtime_error("etcd_cli sync_get failed. {}"_format(resp.error_message()));
        return resp.value().as_string();
    }
    [[maybe_unused]] value_type sync_set(const key_type &key, const value_type &val) {
        auto resp = cli.set(key, val).get();
        if(!resp.is_ok())
            throw std::runtime_error("etcd_cli sync_set failed. {}"_format(resp.error_message()));
        return resp.prev_value().as_string();
    }

    [[gnu::unused]] std::vector<std::pair<key_type, value_type>> cache;
    etcd::Client cli;
};

#endif //ALI_MIDDLEWARE_AGENT_ETCD_SERVICE_HPP
