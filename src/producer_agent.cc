//
// Created by recolic on 18-5-17.
//

#include "producer_agent.hpp"

namespace producer {

    agent::agent(const std::string &etcd_addr_and_port) {
        // TODO: Connect to etcd, register myself. Launch heartbeat thread.
    }

    void agent::listen(const std::string &listen_addr, uint16_t listen_port) {
        // TODO: Launch http server, listen consumer request, and make dubbo request(thread or coroutine).
    }
}
