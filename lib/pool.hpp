#ifndef ALI_MIDDLEW_CONN_POLL_HPP_
#define ALI_MIDDLEW_CONN_POLL_HPP_ 1

#include <rlib/class_decorator.hpp>
#include <thread>
#include <mutex>
#include <utility>
#include <tuple>
#include <functional>
#include <list>
#include <algorithm>
#include <condition_variable>

#include <rlib/stdio.hpp>

using namespace rlib::literals;

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
                explicit iterator(node *ptr) : ptr(ptr) {}
                explicit iterator(T *data_pointer) : ptr(reinterpret_cast<node *>(data_pointer)) {}
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

            ~traceable_list() {
                for (auto iter = begin(); iter != end();) {
                    auto to_release = iter++;
                    delete to_release.ptr;
                }
            }

            iterator begin() {
                return iterator(head, tail);
            }

            iterator end() {
                return iterator((node *) nullptr, tail);
            }

            void push_one(const iterator &where, T &&data, const extra_info_t &extra_info) {
                auto new_node = new node{std::forward<T>(data), nullptr, where.ptr, extra_info};
                ++m_size;
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

            void push_one(const iterator &where, const T &data, const extra_info_t &extra_info) {
                T _data(data);
                push_one(where, std::move(_data), extra_info);
            }

            void push_back(T &&data, const extra_info_t &extra_info) {
                push_one(end(), std::forward<T>(data), extra_info);
            }

            void push_back(const T &data, const extra_info_t &extra_info) {
                push_one(end(), std::forward<T>(data), extra_info);
            }

            void push_front(T &&data, const extra_info_t &extra_info) {
                push_one(begin(), std::forward<T>(data), extra_info);
            }

            void push_front(const T &data, const extra_info_t &extra_info) {
                push_one(begin(), std::forward<T>(data), extra_info);
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
                --m_size;
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

            size_t size() {
                return m_size;
            }

        private:
            node *head = nullptr;
            node *tail = nullptr;
            size_t m_size = 0;
        };
    }

    template<typename obj_t, size_t max_size, typename... _bound_construct_args_t>
    class fixed_object_pool : rlib::noncopyable {
        using buffer_t = impl::traceable_list<obj_t, bool>;
        using this_type = fixed_object_pool<obj_t, max_size, _bound_construct_args_t ...>;
    public:
        explicit fixed_object_pool(_bound_construct_args_t ... _args)
                : _bound_args(std::forward<_bound_construct_args_t>(_args) ...) {
            rlib::println("bound arg2 is {}"_format(std::get<1>(_bound_args)));
        }
//        fixed_object_pool(this_type &&another)
//                : _bound_args(std::forward<_bound_construct_args_t>(another._bound_args) ...),
//                  buffer(std::move(another.buffer)), free_list(std::move(another.free_list)),
//                  buffer_mutex(std::move(another.buffer_mutex))
//        {}

        // `new` an object. Return nullptr if pool is full.
        obj_t *try_borrow_one() {
            std::lock_guard<std::mutex> _l(buffer_mutex);
            return do_try_borrow_one();
        }
        obj_t *borrow_one() {
            auto result = try_borrow_one();
            if(result)
                return result;
            // Not available. Wait for release_one.
            std::unique_lock<std::mutex> lk(buffer_mutex);
            borrow_cv.wait(lk, [this]{return this->new_obj_ready;});
            result = do_try_borrow_one();
            lk.unlock();
            if(!result)
                throw std::logic_error("unknown par error.");
            return result;
        }
        void release_one(obj_t *which) {
            {
                std::lock_guard<std::mutex> _l(buffer_mutex);
                free_list.push_front(which);
                typename buffer_t::iterator elem_iter(which);
                elem_iter.get_extra_info() = true; // mark as free.
                new_obj_ready = true;
            } // lock released.
            borrow_cv.notify_one();
        }

    private:
        std::tuple<_bound_construct_args_t ...> _bound_args;

        buffer_t buffer; // list<obj_t obj, bool is_free>
        std::list<obj_t *> free_list;
        std::mutex buffer_mutex;
        std::condition_variable borrow_cv;
        volatile bool new_obj_ready = false;

        // try_borrow_one without lock. 
        obj_t *do_try_borrow_one() {
            // Optimize here if is performance bottleneck (lockless list... etc...)
            borrow_again:
            if (free_list.size() > 0) {
                // Some object is free. Just return one.
                obj_t *result = *free_list.begin();
                free_list.pop_front();

                typename buffer_t::iterator elem_iter(result);
                elem_iter.get_extra_info() = false; // mark as busy.
                new_obj_ready = false;
                return result;
            }
            if (buffer.size() < max_size) {
                new_obj_to_buffer();
                free_list.push_back(&*--buffer.end());
                goto borrow_again;
            }
            return nullptr;
        }

        // fake emplace_back
        template<size_t ... index_seq>
        inline void new_obj_to_buffer_impl(std::index_sequence<index_seq ...>) {
            buffer.push_back(std::move(obj_t(std::get<index_seq>(_bound_args) ...)), true);
        }

        inline void new_obj_to_buffer() {
            new_obj_to_buffer_impl(std::make_index_sequence<sizeof...(_bound_construct_args_t)>());
        }
    };
}

#endif
