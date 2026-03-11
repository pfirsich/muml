#include "muml.h"

#include <array>
#include <stdio.h>
#include <string_view>
#include <vector>

#include "common.hpp"

struct Sound {
    std::string_view path;
    float volume;
    bool loop;
};

struct Entity {
    std::string_view name;
    int64_t health = 0;
    std::array<float, 2> transform;
    Sound sound;
};

std::string_view sv(muml_string s)
{
    return s.data ? std::string_view { s.data, s.size } : std::string_view {};
}

Entity parse_entity(const muml_node* node)
{
    const auto sprite = muml_child(node, "sprite");
    const auto sound = muml_child(node, "sound");
    Entity entity {
        .name = sv(muml_str(muml_get_arg(muml_child(node, "name"), 0), {})),
        .health = muml_int(muml_get_arg(muml_child(node, "health"), 0), 0),
        .sound = {
            .path = sv(muml_str(muml_get_arg(sound, 0), {})),
            .volume = muml_float(muml_get_attr(sound, "volume"), 1.0f),
            .loop = muml_bool(muml_get_attr(sound, "loop"), false),
        },
    };
    muml_float_n(muml_child(node, "transform"), entity.transform.data(), 2);
    return entity;
}

std::vector<Entity> parse_file(const muml_node* root)
{
    std::vector<Entity> entities;
    for (auto n = root->first_child; n; n = n->next_sibling) {
        if (sv(n->name) == "entity") {
            entities.push_back(parse_entity(n));
        }
    }
    return entities;
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

    const auto entities = parse_file(&nodes[0]);

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