//
// Created by recolic on 18-5-25.
//

#include "etcd_service.hpp"
#include "producer_selector.hpp"

void etcd_service::append(const etcd_service::key_type &key, const etcd_service::value_type &value) {
    /*
     * Append value to list on etcd server. Do it with conn and boost::asio::write.
     * No performance worry, no need for async.
     *
     * boost::asio::write(conn, something);
     */
    auto origin_val = sync_get(key);
    if(!origin_val.empty()) origin_val += '|';
    origin_val += value;
    sync_set(key, value);
}

std::vector<rlib::string> etcd_service::get_list(const etcd_service::key_type &key) {
    /*
     * Query server list from etcd server. Do it with conn and boost::asio::read.
     * No performance worry, no need for async.
     *
     * boost::asio::read(conn, this->cache);
     * return std::cref(this->cache);
     */
    return rlib::string(sync_get(key)).split('|');
}
