#include "wrapper.hpp"

#include <cassert>

namespace muml {

static std::string_view node_name(const muml_node* node)
{
    return node ? sv(node->name) : std::string_view {};
}

static void append_line_prefix(std::string& error, const Context& ctx, const char* p)
{
    error = "line ";
    error.append(std::to_string(ctx.line(p)));
    error.append(": ");
}

void Context::missing_child(const muml_node* owner, const char* child_name)
{
    if (!ok()) {
        return;
    }
    assert(owner && child_name);
    append_line_prefix(error, *this, owner->name.data);
    error.append("node '");
    error.append(node_name(owner));
    error.append("' missing child '");
    error.append(child_name);
    error.append("'");
}

void Context::missing_arg(const muml_node* owner, int index)
{
    if (!ok()) {
        return;
    }
    assert(owner);
    append_line_prefix(error, *this, owner->name.data);
    error.append("node '");
    error.append(node_name(owner));
    error.append("' missing arg[");
    error.append(std::to_string(index));
    error.append("]");
}

void Context::missing_attr(const muml_node* owner, const char* attr_name)
{
    if (!ok()) {
        return;
    }
    assert(owner && attr_name);
    append_line_prefix(error, *this, owner->name.data);
    error.append("node '");
    error.append(node_name(owner));
    error.append("' missing attr '");
    error.append(attr_name);
    error.append("'");
}

void Context::invalid_arg(
    const muml_node* owner, int index, const muml_string* got, const char* expected)
{
    if (!ok()) {
        return;
    }
    assert(owner && got);
    append_line_prefix(error, *this, got->data);
    error.append("node '");
    error.append(node_name(owner));
    error.append("' arg[");
    error.append(std::to_string(index));
    error.append("]: expected ");
    error.append(expected);
    error.append(", got '");
    error.append(sv(*got));
    error.append("'");
}

void Context::invalid_attr(
    const muml_node* owner, const char* attr_name, const muml_string* got, const char* expected)
{
    if (!ok()) {
        return;
    }
    assert(owner && attr_name && got);
    append_line_prefix(error, *this, got->data);
    error.append("node '");
    error.append(node_name(owner));
    error.append("' attr '");
    error.append(attr_name);
    error.append("': expected ");
    error.append(expected);
    error.append(", got '");
    error.append(sv(*got));
    error.append("'");
}

void Context::wrong_count(const muml_node* node, const char* expected, size_t got, size_t want)
{
    if (!ok()) {
        return;
    }
    assert(node && expected);
    append_line_prefix(error, *this, node->name.data);
    error.append("node '");
    error.append(node_name(node));
    error.append("': expected ");
    error.append(expected);
    error.append(" of size ");
    error.append(std::to_string(want));
    error.append(", got size ");
    error.append(std::to_string(got));
}

Node Node::child(const char* name) const
{
    if (!ctx.ok()) {
        return { nullptr, ctx };
    }
    assert(name);
    assert(node && "child() on missing node; use opt_child() and check it first");

    const auto* child = muml_child(node, name);
    if (!child) {
        ctx.missing_child(node, name);
    }
    return { child, ctx };
}

Node Node::opt_child(const char* name) const
{
    if (!ctx.ok()) {
        return { nullptr, ctx };
    }
    assert(name);
    assert(node && "opt_child() on missing node");
    return { muml_child(node, name), ctx };
}

Value Node::arg(int index) const
{
    if (!ctx.ok()) {
        return { nullptr, nullptr, index, nullptr, ctx };
    }
    assert(index >= 0);
    assert(node && "arg() on missing node; use opt_child() and check it first");
    return { muml_get_arg(node, index), node, index, nullptr, ctx };
}

Value Node::attr(const char* name) const
{
    if (!ctx.ok()) {
        return { nullptr, nullptr, -1, name, ctx };
    }
    assert(name);
    assert(node && "attr() on missing node; use opt_child() and check it first");
    return { muml_get_attr(node, name), node, -1, name, ctx };
}

std::string_view Value::get_str() const
{
    if (!ctx.ok()) {
        return {};
    }
    if (!str) {
        if (attr_name) {
            ctx.missing_attr(owner, attr_name);
        } else {
            ctx.missing_arg(owner, arg_index);
        }
        return {};
    }
    return sv(*str);
}

int64_t Value::get_int() const
{
    if (!ctx.ok()) {
        return {};
    }
    if (!str) {
        if (attr_name) {
            ctx.missing_attr(owner, attr_name);
        } else {
            ctx.missing_arg(owner, arg_index);
        }
        return {};
    }

    int64_t ret = 0;
    if (!muml_parse_int(str, &ret)) {
        if (attr_name) {
            ctx.invalid_attr(owner, attr_name, str, "int");
        } else {
            ctx.invalid_arg(owner, arg_index, str, "int");
        }
        return {};
    }
    return ret;
}

float Value::get_float() const
{
    if (!ctx.ok()) {
        return {};
    }
    if (!str) {
        if (attr_name) {
            ctx.missing_attr(owner, attr_name);
        } else {
            ctx.missing_arg(owner, arg_index);
        }
        return {};
    }

    float ret = 0.0f;
    if (!muml_parse_float(str, &ret)) {
        if (attr_name) {
            ctx.invalid_attr(owner, attr_name, str, "float");
        } else {
            ctx.invalid_arg(owner, arg_index, str, "float");
        }
        return {};
    }
    return ret;
}

bool Value::get_bool() const
{
    if (!ctx.ok()) {
        return {};
    }
    if (!str) {
        if (attr_name) {
            ctx.missing_attr(owner, attr_name);
        } else {
            ctx.missing_arg(owner, arg_index);
        }
        return {};
    }

    bool ret = false;
    if (!muml_parse_bool(str, &ret)) {
        if (attr_name) {
            ctx.invalid_attr(owner, attr_name, str, "bool");
        } else {
            ctx.invalid_arg(owner, arg_index, str, "bool");
        }
        return {};
    }
    return ret;
}

std::string_view Value::opt_str(std::string_view fallback) const
{
    if (!ctx.ok()) {
        return fallback;
    }
    return str ? sv(*str) : fallback;
}

int64_t Value::opt_int(int64_t fallback) const
{
    if (!ctx.ok()) {
        return fallback;
    }
    if (!str) {
        return fallback;
    }

    int64_t ret = 0;
    if (!muml_parse_int(str, &ret)) {
        if (attr_name) {
            ctx.invalid_attr(owner, attr_name, str, "int");
        } else {
            ctx.invalid_arg(owner, arg_index, str, "int");
        }
        return fallback;
    }
    return ret;
}

float Value::opt_float(float fallback) const
{
    if (!ctx.ok()) {
        return fallback;
    }
    if (!str) {
        return fallback;
    }

    float ret = 0.0f;
    if (!muml_parse_float(str, &ret)) {
        if (attr_name) {
            ctx.invalid_attr(owner, attr_name, str, "float");
        } else {
            ctx.invalid_arg(owner, arg_index, str, "float");
        }
        return fallback;
    }
    return ret;
}

bool Value::opt_bool(bool fallback) const
{
    if (!ctx.ok()) {
        return fallback;
    }
    if (!str) {
        return fallback;
    }

    bool ret = false;
    if (!muml_parse_bool(str, &ret)) {
        if (attr_name) {
            ctx.invalid_attr(owner, attr_name, str, "bool");
        } else {
            ctx.invalid_arg(owner, arg_index, str, "bool");
        }
        return fallback;
    }
    return ret;
}

}
