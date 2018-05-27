#ifndef ALI_MIDDLEW_CONN_POLL_HPP_
#define ALI_MIDDLEW_CONN_POLL_HPP_ 1

#include <rlib/class_decorator.hpp>
#include <thread>
#include <mutex>
#include <utility>
#include <functional>
#include <deque>
#include <unordered_map>
#include <algorithm>

namespace rlib {
    namespace impl {
        // A double linked list with ability to get iterator from data_t*. Designed for *_object_pool.
        // Only implement few necessary functionalibities.
        template <typename T, typename extra_info_t>
        class traceable_list {
            struct node {
                /*
                 * You may not manage data ownness here. Or you must carefully check if (node*)&data works.
                 */
                T data; // Able to get iterator from T*
                node *prev;
                node *next;
                extra_info_t extra_info; // bool flag. specially designed for object_pool.
            };

        public:
            class iterator : std::bidirectional_iterator_tag {
                friend class traceable_list;
            public:
                iterator(node *ptr) : ptr(ptr) {}

                iterator(T *data_pointer) : ptr(reinterpret_cast<node *>(data_pointer)) {}
                T &operator*() {
                    // If this is an iterator to empty_list.begin(), then nullptr->data throws.
                    return ptr->data;
                }
                T *operator->() {
                    // If this is an iterator to empty_list.begin(), then nullptr->data throws.
                    return &ptr->data;
                }

                extra_info_t &get_extra_info() {
                    return ptr->extra_info;
                }

                const T &operator*() const {
                    // If this is an iterator to empty_list.begin(), then nullptr->data throws.
                    return ptr->data;
                }

                const T *operator->() const {
                    // If this is an iterator to empty_list.begin(), then nullptr->data throws.
                    return &ptr->data;
                }

                const extra_info_t &get_extra_info() const {
                    return ptr->extra_info;
                }

                iterator &operator++() {
                    ptr = ptr->next;
                    return *this;
                }

                const iterator operator++(int) {
                    iterator backup(ptr);
                    operator++();
                    return std::move(backup);
                }

                iterator &operator--() {
                    if (!ptr && _impl_tail)
                        ptr = _impl_tail;
                    else
                        ptr = ptr->prev;
                    return *this;
                }

                const iterator operator--(int) {
                    iterator backup(ptr);
                    operator--();
                    return std::move(backup);
                }

                bool operator==(const iterator &another) const {
                    return ptr == another.ptr;
                }

                bool operator!=(const iterator &another) const {
                    return !operator==(another);
                }
            private:
                node *ptr;

                iterator(node *ptr, node *tail) : ptr(ptr), _impl_tail(tail) {}

                node *_impl_tail = nullptr; // If this iter is created by begin() or end(), it must set this ptr to support end().operator--(), to indicate that this iterator is not invalid.
            };

            iterator begin() {
                return iterator(head, tail);
            }

            iterator end() {
                return iterator((node *) nullptr, tail);
            }

            void push_one(const iterator &where, const T &data, const extra_info_t &extra_info) {
                auto new_node = new node{data, nullptr, where.ptr, extra_info};
                if (!head) {
                    tail = head = new_node;
                    return;
                };
                auto ptr = where.ptr;
                if (!ptr) {
                    // is end();
                    tail->next = new_node;
                    new_node->prev = tail;

                    tail = new_node;
                    return;
                }
                auto left = ptr->prev, right = ptr;
                new_node->prev = right->prev;
                if (left) left->next = new_node;
                right->prev = new_node;

                if (head == ptr)
                    head = new_node;
            }

            void push_back(const T &data, const extra_info_t &extra_info) {
                push_one(end(), data, extra_info);
            }

            void push_front(const T &data, const extra_info_t &extra_info) {
                push_one(begin(), data, extra_info);
            }

            void pop_one(const iterator &which) {
                if (!head)
                    throw std::invalid_argument("nothing to pop.");
                auto ptr = which.ptr;
                if (!ptr) {
                    // end()
                    throw std::invalid_argument("you may not pop end().");
                }
                auto left = ptr->prev, right = ptr->next;

                if (left) left->next = right;
                if (right) right->prev = left;
                if (head == ptr)
                    head = right;
                if (tail == ptr)
                    tail = left;
                delete which.ptr;
            }

            void pop_front() {
                pop_one(begin());
            }

            void pop_back() {
                pop_one(--end());
            }

            void pop_some(const iterator &from, const iterator &to) {
                for (auto iter = from; iter != to;) {
                    auto to_pop = iter++;
                    pop_one(to_pop);
                }
            }

        private:
            node *head = nullptr;
            node *tail = nullptr;
        };
    }

    template <typename obj_t>
    class fixed_object_pool : rlib::noncopyable {
    public:
        fixed_object_pool() = delete;

        //fixed_object_pool() noexcept {}
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