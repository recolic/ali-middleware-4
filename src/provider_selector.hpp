//
// Created by recolic on 18-5-18.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP

#include <etcd_service.hpp>
#include <conn_pool.hpp>
#include <string>
#include <list>
#include <chrono>

#include <rlib/log.hpp>
#include <rlib/macro.hpp>
#include <logger.hpp>

#include <rlib/scope_guard.hpp>

using rlib::literals::operator ""_rs;

#ifdef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP_
#error consumer_agent.hpp must not be included before provider_selector.hpp.
#endif

namespace consumer {

    inline auto http_payload_to_dubbo_parameters(std::string &text) {
        auto kv_str_array = rlib::string(text).split("&");
        std::array<std::string, 4> dubbo_rpc_args;
        for (auto &&kv_str : kv_str_array) {
            auto pos = kv_str.find('=');
            if (pos == std::string::npos) {
                rlog.error("bad kv_Str `{}` skipped"_format(kv_str));
                continue;
            }
            auto key = kv_str.substr(0, pos);
            auto val = kv_str.substr(pos + 1);

            if (key == "interface")
                dubbo_rpc_args[0] = std::move(val);
            else if (key == "method")
                dubbo_rpc_args[1] = std::move(val);
            else if (key == "parameterTypesString")
                dubbo_rpc_args[2] = std::move(val);
            else if (key == "parameter")
                dubbo_rpc_args[3] = std::move(val);
            else
                rlog.error("Warning: got a request with bad body `{}`"_format(text));
        }
        dubbo_rpc_args[2] = rlib::string(std::move(dubbo_rpc_args[2])).replace("%2F", "/").replace("%3B", ";");
        for (auto &s : dubbo_rpc_args) {
            if (s.empty()) {
                rlog.error("Exception detail: par is `{}`"_format(text));
                throw std::runtime_error("Required http argument not provided.");
            }
        }
        return std::move(dubbo_rpc_args);
    }

    // Manage connection to provider_agent servers. You must reuse these connections.
    class provider_info : rlib::noncopyable {
    public:
        provider_info() = delete;

        // Connect to provider_agent and preserve connection.
        provider_info(fd epollfd, std::string addr, uint16_t port)
                : hostname(addr), latency(50000), connpool_latency(1),
                  pconns(new unix_conn_pool(epollfd, addr, port)) {
            pconns->fill_full();
        }

        // Auto-generated move constructor is ambiguous.
        provider_info(provider_info &&another)
                : hostname(std::move(another.hostname)), pconns(std::move(another.pconns)),
                  latency(50000), connpool_latency(1) {

        }

        fd request(fd consumer_conn, std::string &http_req_body) {
            // response will be sent to consumer_conn soon. (really soon!)
            //     your provider connection fd will be returned. You must add it to agent.who_serve_who,
            //          to let provider know who must she deliver its message.
            auto borrowed_conn = pconns->borrow_one();
            rlib_defer([&] { pconns->release_one(borrowed_conn); });
            fd conn = borrowed_conn->get();

            auto dubbo_args = http_payload_to_dubbo_parameters(http_req_body);
            auto req_text = "\n"_rs.join(dubbo_args.begin(), dubbo_args.end());

            rlog.debug("query `{}` sent."_format(req_text));
            RDEBUG_CURR_TIME_VAR(time1);
            rlib::sockIO::sendn_ex(conn, req_text.data(), req_text.size(), MSG_NOSIGNAL);
            RDEBUG_CURR_TIME_VAR(time2);
            RDEBUG_LOG_TIME_DIFF(time2, time1, "send query time");
            return conn;
        }


        const std::string &get_host() const {
            return hostname;
        }

    private:
        std::unique_ptr<unix_conn_pool> pconns;
        std::string hostname;

    public:
        // Other data structure for burden level measurement.
        std::atomic_uint32_t latency; // us
        std::atomic_uint32_t connpool_latency; // us
    };

    class provider_selector : rlib::noncopyable {
    public:
        provider_selector() = delete;

        // Connect to etcd and fetch server list. You must use gRPC or REST API.
        provider_selector(fd epollfd, const std::string &etcd_addr_and_port)
                : epollfd(epollfd), etcd(etcd_addr_and_port) {
            rlog.debug("selector:epollfd={}"_format(epollfd));
            auto addr_list = etcd.get_list("server");
            for (const auto &_addr : addr_list) {
                if (_addr.empty())
                    continue;
                auto addr_and_port = _addr.split(':');
                if (addr_and_port.size() != 2)
                    throw std::runtime_error("Bad server_addr from etcd: `{}`."_format(_addr));
                providers.emplace_back(epollfd, addr_and_port[0], addr_and_port[1].as<uint16_t>());
            }
            rlog.debug("Loaded {} providers."_format(providers.size()));
            if (providers.empty())
                throw std::runtime_error("No provider available. Unable to provide service...");

            for (auto &provider : providers) {
                if (provider.get_host() == "provider-small")
                    robin_queue.push_back(&provider);
                else if (provider.get_host() == "provider-medium") {
                    robin_queue.push_back(&provider);
                    robin_queue.push_back(&provider);
                } else if (provider.get_host() == "provider-large") {
                    robin_queue.push_back(&provider);
                    robin_queue.push_back(&provider);
                    robin_queue.push_back(&provider);
                } else {
                    throw std::runtime_error("naive robin found unknown provider.");
                }
            }
        }

        // Select one provider to query, do auto-balance here.
        // Determine which provider to query. 
        provider_info *query_once() {
            static decltype(robin_queue.begin()) curr = robin_queue.begin();
            size_t cter = 0;
            uint32_t curr_pool_latency = 1;
            for(auto iter = curr; cter < robin_queue.size(); ++cter) {
                if (curr == robin_queue.end())
                    curr = robin_queue.begin();
                curr_pool_latency = (*curr)->connpool_latency.load();
                if(curr_pool_latency < 10000)
                    return *(curr++);
            }
            (*curr)->connpool_latency = curr_pool_latency > 51427 ? (curr_pool_latency - 51427) : 1;
            ++curr;
            return nullptr;
        }

    private:
        fd epollfd;
        etcd_service etcd;

        std::list<provider_info *> robin_queue;
        std::vector<provider_info> providers;

    };

}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
