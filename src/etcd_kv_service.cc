//
// Created by recolic on 18-5-25.
//

#include "etcd_kv_service.hpp"

template<bool is_producer>
void etcd_kv_service<is_producer>::connect(const std::string &etcd_addr_and_port) {
    /*
     * do_connect(this->conn, etcd_addr_and_port);
     * // If you're using REST api, you must open http connection and transfer data by boost::beast
     *    or other http lib in future.
     *    If you're using gRPC, you must call gRPC to build connection.
     *
     *    Confirm if REST api port is allowed by Aliyun Contest document!!! It doesn't seem to be so!
     * // You can do connect synchronously, without performance worry.
     */
}

template<bool is_producer>
const value_type &etcd_kv_service<is_producer>::get(const key_type &key) {
    if (is_producer)
        throw std::invalid_argument("You must not call etcd_kv_service::get in producer.");
    return cache[key];
}

template<bool is_producer>
void etcd_kv_service<is_producer>::set(const key_type &key, const value_type &value) {
    if (!is_producer)
        throw std::invalid_argument("You must not call etcd_kv_service::set in consumer.");
    // Cache is not used. Just write to etcd server!
    // conn.write(your_data); // throws on failure.
}

