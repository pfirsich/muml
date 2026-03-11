#include "muml.h"

#include <string_view>

namespace muml {
inline std::string_view sv(muml_string s)
{
    return s.data ? std::string_view { s.data, s.size } : std::string_view {};
}

struct Value {
    const muml_string* str;

    // required<T>/get<T>, optional<T>/opt<T>

    bool get(bool fallback) const { return muml_bool(str, fallback); }
    int64_t get(int64_t fallback) const { return muml_int(str, fallback); }
    float get(float fallback) const { return muml_float(str, fallback); }
    std::string_view get(std::string_view fallback) const
    {
        return sv(muml_str(str, { fallback.data(), fallback.size() }));
    }

    operator bool() const { return get(false); }
    operator int64_t() const { return get(0l); }
    operator float() const { return get(0.0f); }
    operator std::string_view() const { return get(std::string_view("")); }
};

struct Node {
    const muml_node* node;

    Node child(const char* name) const { return { muml_child(node, name) }; }
    Value operator[](int index) const { return { muml_get_arg(node, index) }; }
    Value operator[](const char* name) const { return { muml_get_attr(node, name) }; }

    template <size_t N>
    std::array<float, N> get_arrayf() const
    {
        std::array<float, N> ret = {};
        muml_float_n(node, ret.data(), N);
        return ret;
    }
};
}