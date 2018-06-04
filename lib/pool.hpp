#ifndef ALI_MIDDLEW_CONN_POLL_HPP_
#define ALI_MIDDLEW_CONN_POLL_HPP_ 1

#include <traceable_list.hpp>
#include <rlib/class_decorator.hpp>
#include <thread>
#include <mutex>
#include <utility>
#include <tuple>
#include <functional>
#include <algorithm>
#include <condition_variable>

#include <boost/asio/spawn.hpp>
#include <boost_asio_event.hpp>
#include <logger.hpp>

namespace rlib {
    /*
     * Multi-threaded object_pool. It will block current thread and wait if
     *     borrow_one() starves, until some other threads release their obj.
     */
    template<typename obj_t, size_t max_size, typename... _bound_construct_args_t>
    class fixed_object_pool : rlib::nonmovable {
        using buffer_t = impl::traceable_list<obj_t, bool>;
        using this_type = fixed_object_pool<obj_t, max_size, _bound_construct_args_t ...>;
    public:
        explicit fixed_object_pool(_bound_construct_args_t ... _args)
                : _bound_args(std::forward<_bound_construct_args_t>(_args) ...) {}

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

    /*
    * Multi-coroutine object_pool(thread safe). It will suspend current coroutine and wait if
    *     borrow_one() starves, until some other coroutines release their obj.
    */
    template<typename obj_t, size_t max_size, typename... _bound_construct_args_t>
    class fixed_object_pool_coro : rlib::nonmovable {
        using buffer_t = impl::traceable_list<obj_t, bool>;
        using this_type = fixed_object_pool_coro<obj_t, max_size, _bound_construct_args_t ...>;
    public:
        explicit fixed_object_pool_coro(boost::asio::io_context &ioc, _bound_construct_args_t ... _args)
                : borrow_avail_event(ioc), _bound_args(std::forward<_bound_construct_args_t>(_args) ...) {}

        // `new` an object. Return nullptr if pool is full.
        obj_t *try_borrow_one() {
            std::lock_guard<std::mutex> _l(buffer_mutex);
            return do_try_borrow_one();
        }

        obj_t *borrow_one(boost::asio::yield_context &yield) {
            auto result = try_borrow_one();
            if (result)
                return result;
            // Not available. Wait for release_one.
            boost::system::error_code ec;
            gt_borrow_one_wait_again:
            borrow_avail_event.async_wait(yield[ec]); // Warning: you must not hold any lock on yield!
            result = try_borrow_one();
            if (!result) {
                // TODO: I'm not sure why this error will occur. This is just a work around.
                // throw std::logic_error("unknown par error. maybe fake asio::event awake?");
                rlog.error("Fake asio::event awake detected!");
                goto gt_borrow_one_wait_again;

            }
            return result;
        }

        void release_one(obj_t *which) {
            {
                std::lock_guard<std::mutex> _l(buffer_mutex);
                free_list.push_front(which);
                typename buffer_t::iterator elem_iter(which);
                elem_iter.get_extra_info() = true; // mark as free.
            } // lock released.
            borrow_avail_event.notify_one();
        }

    private:
        std::tuple<_bound_construct_args_t ...> _bound_args;

        buffer_t buffer; // list<obj_t obj, bool is_free>
        std::list<obj_t *> free_list;
        std::mutex buffer_mutex;
        boost::asio::event borrow_avail_event;

        // try_borrow_one without lock.
        obj_t *do_try_borrow_one() {
            // Optimize here if is performance bottleneck (lockless list... etc...)
            gt_borrow_again:
            if (free_list.size() > 0) {
                // Some object is free. Just return one.
                obj_t *result = *free_list.begin();
                free_list.pop_front();

                typename buffer_t::iterator elem_iter(result);
                elem_iter.get_extra_info() = false; // mark as busy.
                return result;
            }
            if (buffer.size() < max_size) {
                new_obj_to_buffer();
                free_list.push_back(&*--buffer.end());
                goto gt_borrow_again;
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
