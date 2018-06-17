//
// Created by recolic on 18-5-18.
//

#ifndef ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
#define ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP

#include <boost_asio_quick_connect.hpp>
#include <etcd_service.hpp>
#include <conn_pool.hpp>
#include <string>
#include <list>
#include <chrono>

#include <rlib/log.hpp>
#include <rlib/macro.hpp>
#include <logger.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <rlib/scope_guard.hpp>

using rlib::literals::operator ""_rs;

#ifdef ALI_MIDDLEWARE_AGENT_CONSUMER_AGENT_HPP_
#error consumer_agent.hpp must not be included before provider_selector.hpp.
#endif

namespace consumer {
    namespace http = boost::beast::http;
    namespace asio = boost::asio;

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
        using string_body = boost::beast::http::string_body;
    public:
        provider_info() = delete;

        // Connect to provider_agent and preserve connection.
        provider_info(boost::asio::io_context &ioContext, std::string addr, uint16_t port)
                : io_context(ioContext), hostname(addr), latency(50000), connpool_latency(1),
                  pconns(new conn_pool_coro(ioContext, ioContext, addr, (uint16_t) port)) {
            pconns->fill_full();
        }

        // Auto-generated move constructor is ambiguous.
        provider_info(provider_info &&another)
                : io_context(another.io_context), hostname(std::move(another.hostname)),
                  pconns(std::move(another.pconns)), latency(50000), connpool_latency(1) {}

        inline boost::beast::http::response<string_body>
        async_request(boost::beast::http::request<string_body> &req, boost::asio::yield_context &yield) {
            boost::system::error_code ec;
            http::response<string_body> res;
            boost::beast::flat_buffer buffer;

            static const auto resp_500 = [] {
                http::response<http::string_body> res{http::status::internal_server_error, 11};
                res.set(http::field::server, "rHttp");
                res.set(http::field::content_type, "text/plain");
                res.body() = "server error";
                res.prepare_payload();
                return std::move(res);
            }();

            RDEBUG_CURR_TIME_VAR(time1);
            auto borrowed_conn = pconns->borrow_one(yield);
            rlib_defer([&] { pconns->release_one(borrowed_conn); });
            boost::asio::ip::tcp::socket &conn = borrowed_conn->get();
            RDEBUG_CURR_TIME_VAR(time2);
            RDEBUG_LOG_TIME_DIFF(time2, time1, "conn_pool time");

            std::string req_text;
            try {
                auto dubbo_args = http_payload_to_dubbo_parameters(req.body());
                req_text = "\n"_rs.join(dubbo_args.begin(), dubbo_args.end());
            }
            catch (std::exception &e) {
                return resp_500;
            }
            asio::async_write(conn, asio::buffer(req_text), yield[ec]);

            //http::async_write(conn, req, yield[ec]);
            if (ec) {
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
                return resp_500;
            }

            auto time_L = std::chrono::high_resolution_clock::now();
            RDEBUG_LOG_TIME_DIFF(time_L, time2, "write time");
            char resp_buffer[2048]{0};
            conn.async_read_some(asio::buffer(resp_buffer, 2048), yield[ec]);
            //http::async_read(conn, buffer, res, yield[ec]);
            rlog.debug("read: `{}`"_format(resp_buffer));
            if (ec) {
                RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR);
                return resp_500;
            }
            auto time_R = std::chrono::high_resolution_clock::now();
            latency = std::chrono::duration_cast<std::chrono::microseconds>(time_R - time_L).count();
            RDEBUG_LOG_TIME_DIFF(time_R, time_L, "read time");

            res.body() = resp_buffer;
            res.prepare_payload();
            return std::move(res);
        }

        const std::string &get_host() const {
            return hostname;
        }

    private:
        boost::asio::io_context &io_context;
        std::unique_ptr<conn_pool_coro> pconns;
        std::string hostname;

    public:
        // Other data structure for burden level measurement.
        std::atomic_uint32_t latency; // us
        std::atomic_uint32_t connpool_latency; // us
    };

    class provider_selector : rlib::noncopyable {
    public:
        provider_selector() = delete;

#ifdef PROVIDER_SELECTOR_NO_USE_ETCD
        provider_selector(boost::asio::io_context &io_context, const std::string &etcd_addr_and_port)
                : io_context(io_context) {
            rlog.info("(fake_connect) connecting to etcd server {}."_format(etcd_addr_and_port));
            rlog.info("(fake_connect) initializing server list as {}:{}."_format(RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT));
            providers.emplace_back(io_context, RLIB_MACRO_TO_CSTR(DEBUG_SERVER_ADDR), DEBUG_SERVER_PORT);
        }

        provider_info *query_once() {
            return &*providers.begin();
        }
#else

        // Connect to etcd and fetch server list. You must use gRPC or REST API.
        provider_selector(boost::asio::io_context &io_context, const std::string &etcd_addr_and_port)
                : io_context(io_context), etcd(etcd_addr_and_port) {
            auto addr_list = etcd.get_list("server");
            for (const auto &_addr : addr_list) {
                if (_addr.empty())
                    continue;
                auto addr_and_port = _addr.split(':');
                if (addr_and_port.size() != 2)
                    throw std::runtime_error("Bad server_addr from etcd: `{}`."_format(_addr));
                providers.emplace_back(io_context, addr_and_port[0], addr_and_port[1].as<uint16_t>());
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
        etcd_service etcd;
        // TODO: select the one with lowest latency.
        std::list<provider_info *> robin_queue;
#endif


    private:
        std::vector<provider_info> providers;
        boost::asio::io_context &io_context;

    };

}


#endif //ALI_MIDDLEWARE_AGENT_PRODUCER_SELECTOR_HPP
