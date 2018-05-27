#ifndef ALI_MIDDLEW_CONN_POLL_HPP_
#define ALI_MIDDLEW_CONN_POLL_HPP_ 1

#include <rlib/class_decorator.hpp>
#include <thread>
#include <mutex>
#include <utility>
#include <functional>
#include <deque>
#include <unordered_map>

namespace rlib {
    namespace impl {
        // A double linked list with ability to get iterator from data_t*. Designed for *_object_pool.
        // Only implement few necessary functionalibities.
        template <typename T, typename extra_info_t>
        class traceable_list {
            struct node {
                node prev;
                node next;
                T data;
                extra_info_t extra_info; // bool flag. specially designed for object_pool.
            };

        public:
            class iterator : std::bidirectional_iterator_tag {
            public:
                iterator(node *data_pointer) : ptr(data_pointer) {}
                T &operator*() {
                    return ptr->data;
                }
                T *operator->() {
                    return &ptr->data;
                }
                iterator &operator++() {
                    ptr = ptr->next;
                    return *this;
                }

            private:
                node *ptr;
            };
        };
    }

    template <typename obj_t>
    class fixed_object_pool : rlib::noncopyable {
    public:
        fixed_object_pool() = delete;
        fixed_object_pool() noexcept {}
        void preserve(size_t size) {}

        // `new` an object. Blocked if pool is full.
        obj_t *create_one() {}
        void release_one(obj_t *which) {}

    private:
        size_t size = 0;
        std::unordered_map<obj_t *, std::pair<bool, obj_t>> buffer;
    };
}

#endif