//
// Created by recolic on 18-5-25.
//

#include "etcd_service.hpp"
#include <Client.hpp>
#include <rpc.grpc.pb.h>
#include <etcdserver.pb.h>

void etcd_service::connect(const std::string &etcd_addr_and_port) {
    etcd::Client etcd(etcd_addr_and_port);
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

void etcd_service::append(const std::string etcd_addr_and_port ,const etcd_service::key_type &key, const etcd_service::value_type &value) {
    /*
     * Append value to list on etcd server. Do it with conn and boost::asio::write.
     * No performance worry, no need for async.
     *
     * boost::asio::write(conn, something);
     */
    etcd::Client etcd(etcd_addr_and_port);
    std::string addr="/test/";
    addr=addr+std::string(key);
    std::pplx::task<etcd::Response>response_task=etcd.set(addr,std::string(value));
    try {
        etcd::Response response = response_task.get();
        if (response.is_ok())
            std::cout << "The new value is successfully set, previous value was "
                      << response.prev_value().as_string();
        else
            std::cout << "operation failed, details: " << response.error_message();
    }
    catch (std::ecxeption const & ex) {
        std::cerr << "communication problem, details: " << ex.what();
    }
}

const etcd_service::container_type &etcd_service::get_list(const std::string etcd_addr_and_port,const etcd_service::key_type &key) {
    /*
     * Query server list from etcd server. Do it with conn and boost::asio::read.
     * No performance worry, no need for async.
     *
     * boost::asio::read(conn, this->cache);
     * return std::cref(this->cache);
     */
    container_type server;
    etcd::Client etcd(etcd_addr_and_port);
    std::string addr="/test/new_dir";
    etcd::Response resp = etcd.ls(addr).get();
    for(int i = 0;i <resp.keys().size();++ i){
        server.push_back(std::pair{resp.key(),resp.value()});
    }
    return container_type();
}
