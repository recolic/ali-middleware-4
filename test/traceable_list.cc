//
// Created by recolic on 18-5-27.
//

#include <pool.hpp>
#include <rlib/stdio.hpp>

using namespace rlib;

int main() {
    rlib::impl::traceable_list<int, bool> ls;
    using iterator_t = decltype(ls.begin());
    ls.push_back(3212, true);
    ls.push_back(312, true);
    ls.push_back(3312, true);
    ls.push_back(32412, false);
    ls.push_back(3712, true);
    ls.pop_back();
    ls.pop_front();
    ls.push_back(32122, true);
    ls.push_back(2222, true);
    ls.push_back(99, false);
    ls.push_back(42, true);
    ls.push_back(991, true);
    ls.pop_one(++ ++ ++ls.begin());
    ls.push_back(2222, true);
    ls.push_back(2222, true);
    ls.push_back(2222, true);
    std::for_each(ls.begin(), ls.end(), [](int &i) {
        if (iterator_t(&i).get_extra_info()) {
            println(i);
        }
    });
    return 0;
}

