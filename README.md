# muml - Micro Markup Language

Sometimes a key-value file (ini-style) is not enough and you need something more expressive, but you really don't want a big parser for something fancy, that you barely understand.
muml tries to be very simple to understand and *very* simple to parse.

Without the boilerplate this muml parser in C++20 is less than 150 lines.

This is not for big serious software projects, but for when you just want to put some data in a file and read it and don't include a crazy file format and a crazy library to do so.

## Concept

```
# comment
entity {
    name "player"
    health 100
    transform 0.0 0.0
    sprite "player.png" layer=2
    sound "footstep.wav" volume=0.5 loop=true
}

entity {
    name "enemy_grunt"
    health 30
    transform 400.0 200.0
    sprite "grunt.png" layer=2
    collider 16 24
}

entity {
    name "chest"
    transform 128.0 64.0
    sprite "chest_closed.png" layer=1
    trigger radius=8.0
}
```

The general structure of a muml document is a tree of nodes, each of which has a name, any number of arguments (referenced by index) and any number of attributes (referenced by name).

Every one of these (name, argument, attribute name and attribute value are all "strings"), which means they are either quoted (single or double quotes allowed) or unquoted and terminated by whitespace.

This is heavily inspired by [KDL](https://kdl.dev/), [SDLang](https://sdlang.org/) and [Styx](https://styx.bearcove.eu/).

You can represent a muml file like this:
```
struct Node {
	string name;
	Array<string> args;
	Array<(string, string)> attrs;
	Array<Node> children;
};
```

If you need your strings to mean something (like numbers or bools or whatever), you should parse them yourself, though this library includes some helper functions for this.
