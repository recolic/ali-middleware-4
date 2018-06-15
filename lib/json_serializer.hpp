//
// Created by recolic on 18-5-16.
//

#ifndef ALI_MIDDLEWARE_AGENT_JSON_SERIALIZER_HPP
#define ALI_MIDDLEWARE_AGENT_JSON_SERIALIZER_HPP

#include <kv_serializer.hpp>
#include <rlib/string.hpp>
#include <sstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using rlib::literals::operator""_format;
using boost::property_tree::ptree;
using boost::property_tree::read_json;

class json_serializer : public kv_serializer
{
public:
    virtual ~json_serializer() = default;
    virtual std::string serialize(const kv_list_t &kv_list) const {
        if(kv_list.empty())
            return std::string("{}");
        std::string result = "{\n";
        for(const auto &kv : kv_list) {
            result += "\"{}\":\"{}\",\n"_format(kv.first, kv.second);
        }
        result[result.size() - 2] = '\n';
        result[result.size() - 1] = '}';
        return std::move(result);
    }
    virtual kv_list_t deserialize(const std::string &json) const {
        ptree pt;
        std::stringstream ss(json);
        read_json(ss, pt);

        kv_list_t result;
        std::for_each(pt.begin(), pt.end(), [&result](auto pos){
            // Iterate over only one level.
            std::pair<key_t, value_t> kv {pos.first, pos.second.get_value(std::string(""))};
            result.push_back(kv);
        });
        return std::move(result);
    }
};


#endif //ALI_MIDDLEWARE_AGENT_JSON_SERIALIZER_HPP
