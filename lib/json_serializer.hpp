//
// Created by recolic on 18-5-16.
//

#ifndef ALI_MIDDLEWARE_AGENT_JSON_SERIALIZER_HPP
#define ALI_MIDDLEWARE_AGENT_JSON_SERIALIZER_HPP

#include "kv_serializer.hpp"
#include <rlib/string.hpp>

class json_serializer : public kv_serializer
{
    using namespace rlib::literals;

public:
    virtual ~json_serializer() = default;
    virtual static std::string serialize(const kv_list_t &kv_list) {
        if(kv_list.empty())
            return std::string("{}");
        std::string result = "{";
        for(const auto &kv : kv_list) {
            result += "\"{}\": \"{}\","_format(kv.first, kv.second);
        }
        result[result.size() - 1] = '}';
        return std::move(result);
    }
};


#endif //ALI_MIDDLEWARE_AGENT_JSON_SERIALIZER_HPP
