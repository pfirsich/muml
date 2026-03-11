#include "muml.h"

#include <cassert>
#include <charconv>
#include <span>
#include <string_view>

#if defined(WIN32)
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C"
#endif

struct Parser {
    const char* cur;
    const char* end;
    uint32_t line = 1;
    std::span<muml_node> nodes;
    size_t node_count = 0;
    std::span<muml_string> args;
    size_t arg_count = 0;
    std::span<muml_attr> attrs;
    size_t attr_count = 0;
    const char* error_message = nullptr;
};

static muml_node* alloc_node(Parser& p)
{
    if (p.node_count >= p.nodes.size()) {
        p.error_message = "too many nodes";
        return nullptr;
    }
    return &p.nodes[p.node_count++];
}

static muml_string* alloc_arg(Parser& p)
{
    if (p.arg_count >= p.args.size()) {
        p.error_message = "too many args";
        return nullptr;
    }
    return &p.args[p.arg_count++];
}

static muml_attr* alloc_attr(Parser& p)
{
    if (p.attr_count >= p.attrs.size()) {
        p.error_message = "too many attrs";
        return nullptr;
    }
    return &p.attrs[p.attr_count++];
}

static bool is_hspace(char c)
{
    return c == ' ' || c == '\t';
}

static bool is_space(char c)
{
    // I have had it with \f and \v and whatever I am not thinking of.
    // I handle these guys for more than a decade and I have yet to see them. No more!
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_special(char c)
{
    // clang-format off
    return is_space(c) // separates nodes, args and attrs
        || c == '#' // starts a comment
        || c == '=' // separates attr name and value
        || c == '{' // starts child block
        || c == '}' // ends child block
        || c == '\'' // quote (strings)
        || c == '"'; // quote (strings)
    // clang-format on
}

static bool can_start_string(char c)
{
    return c == '\'' || c == '"' || !is_special(c);
}

static void skip_ws(Parser& p)
{
    while (p.cur < p.end) {
        if (*p.cur == '#') {
            while (p.cur < p.end && *p.cur != '\n') {
                p.cur++;
            }
        } else if (is_space(*p.cur)) {
            if (*p.cur == '\n') {
                p.line++;
            }
            p.cur++;
        } else {
            break;
        }
    }
}

static bool skip_hws(Parser& p)
{
    const auto start = p.cur;
    while (p.cur < p.end && is_hspace(*p.cur)) {
        p.cur++;
    }
    return start != p.cur;
}

static muml_string parse_string(Parser& p)
{
    if (p.cur < p.end && (*p.cur == '\'' || *p.cur == '"')) {
        const auto open_char = *p.cur;
        p.cur++;
        const char* start = p.cur;
        while (p.cur < p.end && *p.cur != open_char) {
            if (*p.cur == '\n') {
                p.line++;
            }
            p.cur++;
        }
        if (p.cur >= p.end) {
            p.error_message = "unterminated string";
            return {};
        }
        const muml_string s = { start, (size_t)(p.cur - start) };
        p.cur++; // skip closing char
        return s;
    }

    const char* start = p.cur;
    while (p.cur < p.end && !is_special(*p.cur)) {
        p.cur++;
    }
    if (p.cur == start) {
        p.error_message = "expected string";
        return {};
    }
    return { start, (size_t)(p.cur - start) };
}

static bool parse_children(Parser& p, muml_node* parent);

static bool parse_node(Parser& p, muml_node* node)
{
    *node = {};
    node->name = parse_string(p);
    if (!node->name.data) {
        return false;
    }

    // args and attrs for a single node are contiguous
    node->args = &p.args[p.arg_count];
    node->attrs = &p.attrs[p.attr_count];

    while (p.cur < p.end) {
        const auto skipped = skip_hws(p);

        if (p.cur >= p.end || !can_start_string(*p.cur)) {
            break;
        }

        // values must be separated by whitespace
        if (!skipped) {
            p.error_message = "expected whitespace";
            return false;
        }

        const auto str = parse_string(p);
        if (!str.data) {
            return false;
        }

        // if the next char is a '=' we are parsing an attribute, not an argument
        if (p.cur < p.end && *p.cur == '=') {
            p.cur++;
            auto* attr = alloc_attr(p);
            if (!attr) {
                return false;
            }
            attr->name = str;
            attr->value = parse_string(p);
            if (!attr->value.data) {
                return false;
            }
            node->attr_count++;
        } else {
            auto* arg = alloc_arg(p);
            if (!arg) {
                return false;
            }
            *arg = str;
            node->arg_count++;
        }
    }

    if (p.cur < p.end && *p.cur == '{') {
        p.cur++;
        if (!parse_children(p, node)) {
            return false;
        }
        skip_ws(p);
        if (p.cur >= p.end || *p.cur != '}') {
            p.error_message = "expected '}'";
            return false;
        }
        p.cur++;
    }

    return true;
}

static bool parse_children(Parser& p, muml_node* parent)
{
    muml_node* last_child = nullptr;
    while (true) {
        skip_ws(p);
        if (p.cur >= p.end || *p.cur == '}') {
            break;
        }

        muml_node* node = alloc_node(p);
        if (!node) {
            return false;
        }

        if (last_child) {
            last_child->next_sibling = node;
        } else {
            parent->first_child = node;
        }
        last_child = node;

        if (!parse_node(p, node)) {
            return false;
        }
    }
    return true;
}

EXPORT muml_parse_result muml_parse(const char* data, size_t size, muml_node* nodes,
    size_t max_node_count, muml_string* args, size_t max_arg_count, muml_attr* attrs,
    size_t max_attr_count)
{
    assert(max_node_count > 0 && max_arg_count > 0 && max_attr_count > 0);

    Parser p = {
        .cur = data,
        .end = data + size,
        .nodes = { nodes, max_node_count },
        .args = { args, max_arg_count },
        .attrs = { attrs, max_attr_count },
    };

    // nodes[0] is root
    nodes[0] = {};
    p.node_count = 1;
    if (!parse_children(p, &nodes[0])) {
        return { p.cur, p.line, p.error_message };
    }

    skip_ws(p);
    if (p.cur < p.end) {
        p.error_message = "expected end of input";
    }

    return { p.cur, p.line, p.error_message };
}

// Helpers

EXPORT const muml_node* muml_child(const muml_node* node, const char* name)
{
    if (!node) {
        return nullptr;
    }
    const std::string_view name_sv = name;
    for (auto c = node->first_child; c; c = c->next_sibling) {
        if (name_sv == std::string_view { c->name.data, c->name.size }) {
            return c;
        }
    }
    return nullptr;
}

EXPORT const muml_string* muml_get_arg(const muml_node* node, uint32_t index)
{
    return node && index < node->arg_count ? &node->args[index] : nullptr;
}

EXPORT const muml_string* muml_get_attr(const muml_node* node, const char* name)
{
    if (!node) {
        return nullptr;
    }
    const std::string_view name_sv = name;
    for (const auto& attr : std::span { node->attrs, node->attr_count }) {
        if (name_sv == std::string_view { attr.name.data, attr.name.size }) {
            return &attr.value;
        }
    }
    return nullptr;
}

template <typename T>
bool parse_number(const muml_string* s, T& v)
{
    v = {};
    const auto [ptr, ec] = std::from_chars(s->data, s->data + s->size, v);
    if (ec != std::errc()) {
        return false;
    }
    if (ptr != s->data + s->size) {
        return false;
    }

    return true;
}

EXPORT bool muml_parse_float(const muml_string* s, float* out)
{
    assert(s && out);
    return parse_number(s, *out);
}

EXPORT bool muml_parse_int(const muml_string* s, int64_t* out)
{
    assert(s && out);
    return parse_number(s, *out);
}

EXPORT bool muml_parse_bool(const muml_string* s, bool* out)
{
    assert(s && out);
    const std::string_view sv { s->data, s->size };
    if (sv == "true") {
        *out = true;
        return true;
    } else if (sv == "false") {
        *out = false;
        return true;
    }
    return false;
}

EXPORT float muml_float(const muml_string* s, float fallback)
{
    float v = 0.0f;
    return s && muml_parse_float(s, &v) ? v : fallback;
}

EXPORT bool muml_float_n(const muml_node* n, float* values, size_t num)
{
    if (!n || num > n->arg_count) {
        return false;
    }
    for (size_t i = 0; i < num; ++i) {
        if (!muml_parse_float(&n->args[i], &values[i])) {
            return false;
        }
    }
    return true;
}

EXPORT int64_t muml_int(const muml_string* s, int64_t fallback)
{
    int64_t v = 0;
    return s && muml_parse_int(s, &v) ? v : fallback;
}

EXPORT bool muml_int_n(const muml_node* n, int64_t* values, size_t num)
{
    if (!n || num > n->arg_count) {
        return false;
    }
    for (size_t i = 0; i < num; ++i) {
        if (!muml_parse_int(&n->args[i], &values[i])) {
            return false;
        }
    }
    return true;
}

EXPORT bool muml_bool(const muml_string* s, bool fallback)
{
    bool v = false;
    return s && muml_parse_bool(s, &v) ? v : fallback;
}

EXPORT muml_string muml_str(const muml_string* s, muml_string fallback)
{
    return s ? *s : fallback;
}