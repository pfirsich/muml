#pragma once

#include "muml.h"

#include <array>
#include <cassert>
#include <string>
#include <string_view>

namespace muml {

inline std::string_view sv(muml_string s)
{
    return s.data ? std::string_view { s.data, s.size } : std::string_view {};
}

struct Context {
    // source must be the same buffer that was passed to muml_parse().
    Context(const char* source, size_t size) : source(source), size(size) { }

    Context() = delete;

    const char* source;
    size_t size;
    std::string error;

    bool ok() const { return error.empty(); }
    uint32_t line(const char* p) const { return muml_get_line(source, size, p); }

    void missing_child(const muml_node* owner, const char* child_name);
    void missing_arg(const muml_node* owner, int index);
    void missing_attr(const muml_node* owner, const char* attr_name);
    void invalid_arg(
        const muml_node* owner, int index, const muml_string* got, const char* expected);
    void invalid_attr(const muml_node* owner, const char* attr_name, const muml_string* got,
        const char* expected);
    void wrong_count(const muml_node* node, const char* expected, size_t got, size_t want);
};

struct Value {
    const muml_string* str = nullptr;
    const muml_node* owner = nullptr;
    int arg_index = -1;
    const char* attr_name = nullptr;
    Context& ctx;

    explicit operator bool() const { return str; }

    std::string_view get_str() const;
    int64_t get_int() const;
    float get_float() const;
    bool get_bool() const;

    std::string_view opt_str(std::string_view fallback = {}) const;
    int64_t opt_int(int64_t fallback = {}) const;
    float opt_float(float fallback = {}) const;
    bool opt_bool(bool fallback = {}) const;
};

struct Node {
    const muml_node* node = nullptr;
    Context& ctx;

    explicit operator bool() const { return node; }

    // child() is required: it reports an error immediately if the child is missing.
    Node child(const char* name) const;
    // opt_child() is optional: it returns a null node and the caller must check it.
    Node opt_child(const char* name) const;
    Value arg(int index) const;
    Value attr(const char* name) const;

    template <size_t N>
    std::array<float, N> get_arrayf() const
    {
        std::array<float, N> ret = {};
        if (!ctx.ok()) {
            return ret;
        }
        assert(node && "get_arrayf() on missing node; use opt_child() and check it first");
        if (node->arg_count != N) {
            ctx.wrong_count(node, "float array", node->arg_count, N);
            return ret;
        }
        for (size_t i = 0; i < N; ++i) {
            if (!muml_parse_float(&node->args[i], &ret[i])) {
                ctx.invalid_arg(node, (int)i, &node->args[i], "float");
                return ret;
            }
        }
        return ret;
    }

    template <size_t N>
    std::array<float, N> opt_arrayf(std::array<float, N> fallback = {}) const
    {
        std::array<float, N> ret = {};
        if (!ctx.ok()) {
            return fallback;
        }
        assert(node && "opt_arrayf() on missing node; use opt_child() and check it first");
        if (node->arg_count != N) {
            ctx.wrong_count(node, "float array", node->arg_count, N);
            return fallback;
        }
        for (size_t i = 0; i < N; ++i) {
            if (!muml_parse_float(&node->args[i], &ret[i])) {
                ctx.invalid_arg(node, (int)i, &node->args[i], "float");
                return fallback;
            }
        }
        return ret;
    }
};

}
