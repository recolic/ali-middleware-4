//
// Created by recolic on 18-5-25.
//

#ifndef ALI_MIDDLEWARE_AGENT_ETCD_KV_SERVICE_HPP
#define ALI_MIDDLEWARE_AGENT_ETCD_KV_SERVICE_HPP


#include <string>
#include <rlib/class_decorator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <unordered_map>

/*
 * This service can be run in seperate thread, so async IO is not required, as that's not a performance issue(even in the future).
 * However, get and set must be carefully optimized.
 *
 * Note that, currently providers is not really `dynamically` registered and discovered.
 * So providers just call set one by one, and consumer call get and decode server list.
 */
template<bool is_provider>
class [[deprecated]] etcd_kv_service : rlib::noncopyable {
public:
    using key_type = std::string;
    using value_type = std::string; // Maybe you want to set value_type to list<string>. Implement it by yourself!

    etcd_kv_service() = delete;

    etcd_kv_service(const std::string &etcd_addr_and_port) : conn(io_context) {
        connect(etcd_addr_and_port);
        if (is_provider)
            set("providers", "my_addr:my_port");
        else
            pull_cache();
    }

    void connect(const std::string &etcd_addr_and_port);

    // Maybe you want to append.
    void set(const key_type &key, const value_type &value);
    // Maybe you want to get_list.
    const value_type &get(const key_type &key);
    // Implement what you want by yourself.

private:
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket conn;

    // Download data to cache.
    void pull_cache();
    // void push_cache(); // Maybe for future usage.
    std::unordered_map<key_type, value_type> cache;

    /* For future usage: This function must launch a seperate thread, intervally query etcd to check if there's any
         status update, and update cache.
public:
    void launch_daemon();
private:
    [[noreturn]] void daemon_proc();
    std::thread daemon_thread;
    std::mutex cache_lock_mut;
    std::lock<std::mutex> cache_lock;
    */
};


#endif //ALI_MIDDLEWARE_AGENT_ETCD_KV_SERVICE_HPP
