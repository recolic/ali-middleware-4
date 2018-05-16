#include <json_serializer.hpp>
#include <rlib/stdio.hpp>
using namespace std;
using namespace rlib;
int main() {
    auto json = json_serializer().serialize(kv_serializer::kv_list_t{{"firstName", "Recolic"}, {"lastName", "Keghart"}, {"fuck", "shit"}});
    println(json);
    auto ls = json_serializer().deserialize(json);
    for(auto ele : ls) {
        println(ele.first, ele.second);
    }
    return 0;
}