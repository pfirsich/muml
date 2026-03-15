#include <array>
#include <stdio.h>
#include <string_view>
#include <vector>

#include "common.hpp"
#include "wrapper.hpp"

using namespace muml;

struct Sound {
    std::string_view path;
    float volume = 1.0f;
    bool loop = false;
};

struct Entity {
    std::string_view name;
    std::array<float, 2> transform;
    int64_t health = 0;
    Sound sound;
};

Entity parse_entity(Node node)
{
    Entity result {
        .name = node.child("name").arg(0).get_str(),
        .transform = node.child("transform").get_arrayf<2>(),
    };

    if (const auto health = node.opt_child("health")) {
        result.health = health.arg(0).opt_int(0);
    }

    if (const auto sound = node.opt_child("sound")) {
        result.sound = {
            .path = sound.arg(0).get_str(),
            .volume = sound.attr("volume").opt_float(1.0f),
            .loop = sound.attr("loop").opt_bool(false),
        };
    }
    return result;
}

std::vector<Entity> parse_file(const muml_node* root, Context& ctx)
{
    std::vector<Entity> entities;
    for (auto n = root->first_child; n; n = n->next_sibling) {
        if (sv(n->name) == "entity") {
            const auto entity = parse_entity({ n, ctx });
            if (!ctx.ok()) {
                break;
            }
            entities.push_back(entity);
        }
    }
    return entities;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: parse-entities-cpp FILE\n");
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

    Context ctx(data.data(), data.size());
    const auto entities = parse_file(&nodes[0], ctx);
    if (!ctx.ok()) {
        fprintf(stderr, "%s\n", ctx.error.c_str());
        return 1;
    }

    for (const auto& e : entities) {
        printf("name: %.*s\n", (int)e.name.size(), e.name.data());
        printf("health: %ld\n", e.health);
        printf("sound.path: %.*s\n", (int)e.sound.path.size(), e.sound.path.data());
        printf("sound.volume: %f\n", e.sound.volume);
        printf("sound.loop: %d\n", e.sound.loop);
        printf("transform: %f, %f\n", e.transform[0], e.transform[1]);
        printf("\n");
    }
}
