//
// Created by recolic on 18-6-16.
//

#include <dubbo_client.hpp>
#include <boost/asio.hpp>
using namespace boost::asio;

rlib::logger rlog(std::cout);

int main() {
    rlog.set_log_level(rlib::log_level_t::DEBUG);
    io_context ioc;
    dubbo_client dc(ioc, "127.0.0.1", 20880);
    kv_serializer::kv_list_t kvl {
            {"interface", "com.alibaba.dubbo.performance.demo.provider.IHelloService"},
            {"method", "hash"},
            {"parameterTypesString", "Ljava/lang/String;"},
            {"parameter", "fuckshit"}
    };

    auto ret = dc.sync_request("com.alibaba.dubbo.performance.demo.provider.IHelloService","hash","Ljava/lang/String;","fuckshit");
    rlog.debug("ret.status={}, payload=`{}`"_format((int)ret.status, ret.value));
}
