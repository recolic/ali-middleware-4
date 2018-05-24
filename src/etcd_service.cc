//
// Created by recolic on 18-5-25.
//

#include "etcd_service.hpp"

void etcd_service::connect(const std::string &etcd_addr_and_port) {
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

void etcd_service::append(const etcd_service::key_type &key, const etcd_service::value_type &value) {
    /*
     * Append value to list on etcd server. Do it with conn and boost::asio::write.
     * No performance worry, no need for async.
     *
     * boost::asio::write(conn, something);
     */
}

const etcd_service::container_type &etcd_service::get_list(const etcd_service::key_type &key) {
    /*
     * Query server list from etcd server. Do it with conn and boost::asio::read.
     * No performance worry, no need for async.
     *
     * boost::asio::read(conn, this->cache);
     * return std::cref(this->cache);
     */
    return container_type();
}
