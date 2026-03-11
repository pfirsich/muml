#include "muml.h"

#include <array>
#include <stdio.h>

#include "common.hpp"

void print_node(muml_node& node, uint32_t indent = 0)
{
    const auto indent_size = 2;
    printf("%*sNODE %.*s\n", indent * indent_size, "", (int)node.name.size, node.name.data);
    for (size_t i = 0; i < node.arg_count; ++i) {
        printf("%*s'%.*s'\n", (indent + 1) * indent_size, "", (int)node.args[i].size,
            node.args[i].data);
    }
    for (size_t i = 0; i < node.attr_count; ++i) {
        printf("%*s'%.*s' = '%.*s'\n", (indent + 1) * indent_size, "", (int)node.attrs[i].name.size,
            node.attrs[i].name.data, (int)node.attrs[i].value.size, node.attrs[i].value.data);
    }
    for (auto n = node.first_child; n; n = n->next_sibling) {
        print_node(*n, indent + 1);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: dump FILE\n");
        return 1;
    }

    const auto data = read_file(argv[1]);

    std::array<muml_node, 1024> nodes;
    std::array<muml_string, 1024> args;
    std::array<muml_attr, 1024> attrs;
    const auto res = muml_parse(data.data(), data.size(), nodes.data(), nodes.size(), args.data(),
        args.size(), attrs.data(), attrs.size());

    if (res.error_message) {
        fprintf(stderr, "%u: %s\n", res.error_line, res.error_message);
        return 1;
    }

    for (auto n = nodes[0].first_child; n; n = n->next_sibling) {
        print_node(*n, 0);
    }
}