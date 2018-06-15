//
// Created by recolic on 18-5-18.
//

#include <provider_selector.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

consumer::provider_info &consumer::provider_selector::query_once() {
    static size_t curr = 0;
    if(curr == providers.size())
        curr = 0;
    return providers[curr++];
}
