//
// Created by recolic on 18-5-27.
//

#include <pool.hpp>
#include <rlib/stdio.hpp>
#include <rlib/log.hpp>
#include <ext/concurrence.h>

using namespace rlib::literals;
using namespace std::literals;

int cter = 11;
rlib::logger rlog(std::cout);

class test_obj_cls {
public:
    test_obj_cls() : i(cter), j(cter * cter) { ++cter; }

    void show() {
        rlog.info("i={} j={}"_format(i, j));
    }


private:
    int i = 2;
    int j = 4;
};

int main() {
    rlib::fixed_object_pool<test_obj_cls> pool(4);
    auto obj1 = pool.borrow_one();
    auto obj2 = pool.borrow_one();
    auto obj3 = pool.borrow_one();
    obj1->show();
    obj2->show();
    obj3->show();
    rlog.info("borrow done");

    pool.release_one(obj1);
    auto _obj1 = pool.borrow_one();
    auto _obj2 = pool.borrow_one(); // full
    _obj1->show();
    _obj2->show();
    rlog.info("release&borrow done");

    std::thread th([&] {
        rlog.info("borrowing...");
        auto obj = pool.borrow_one();
        rlog.info("done! message is");
        obj->show();
    });

    std::this_thread::sleep_for(1s);
    pool.release_one(_obj2);
    th.join();
}
