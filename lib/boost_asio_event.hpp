#ifndef _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_EVENT_HPP
#define _ALI_MIDDLEWARE_AGENT_BOOST_ASIO_EVENT_HPP 1

#include <boost/asio.hpp>

namespace boost {
    namespace asio {
        /*
         * event-based condition-variable like, boost async interface.
         * which is designed to support coroutine-level event-based programming.
         */
        class event : boost::noncopyable {
        public:
            explicit event(boost::asio::io_context &io_context)
                    : timer_(io_context) {
                // Setting expiration to infinity will cause handlers to
                // wait on the timer until cancelled.
                timer_.expires_at(boost::posix_time::pos_infin);
            }

            template<typename WaitHandler>
            void async_wait(WaitHandler &&handler) {
                // bind is used to adapt the user provided handler to the deadline
                // timer's wait handler type requirement.
                timer_.async_wait(std::forward<WaitHandler &&>(handler));
            }

            void notify_one() { timer_.cancel_one(); }

            void notify_all() { timer_.cancel(); }

        private:
            boost::asio::deadline_timer timer_;
        };
    }
}

#endif //_ALI_MIDDLEWARE_AGENT_BOOST_ASIO_EVENT_HPP
