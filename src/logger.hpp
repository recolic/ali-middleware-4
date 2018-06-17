#ifndef _ALI_MIDDLEWARE_AGENT_LOGGER_HPP
#define _ALI_MIDDLEWARE_AGENT_LOGGER_HPP 1

#include <rlib/log.hpp>

extern rlib::logger rlog;
using rlib::literals::operator ""_format;

#define ON_BOOST_FATAL(ec) do { RBOOST_LOG_EC(ec, rlib::log_level_t::FATAL); \
        throw std::runtime_error(ec.message()); } while(false)
#define ON_BOOST_ERROR(ec) do { RBOOST_LOG_EC(ec, rlib::log_level_t::ERROR); \
        return; } while(false)
#define RBOOST_LOG_EC(ec, level) rlog.log("boost error at {}:{}, {}"_format(__FILE__, __LINE__, ec.message()), level)

#define RDEBUG_CURR_TIME_VAR(var_name) auto var_name = std::chrono::high_resolution_clock::now()
#define RDEBUG_LOG_TIME_DIFF(time_R, time_L, cstr_name) rlog.debug(cstr_name" used {} us."_format(std::chrono::duration_cast<std::chrono::microseconds>(time_R - time_L).count()))

#endif //_ALI_MIDDLEWARE_AGENT_LOGGER_HPP
