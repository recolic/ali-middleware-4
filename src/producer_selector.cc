//
// Created by recolic on 18-5-18.
//

#include <producer_selector.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
<<<<<<< HEAD

consumer::producer_info &consumer::producer_selector::query_once() {
    static size_t curr = 0;
    if(curr == producers.size())
        curr = 0;
    return producers[curr++];
}

=======
>>>>>>> adapting-etcd-cpp-api
