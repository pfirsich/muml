# I let ChatGPT generate this based on muml.cpp


class Node:
    def __init__(
        self,
        name: str,
        args: list | None = None,
        attrs: list[tuple[str, str]] | None = None,
        children: list["Node"] | None = None,
    ):
        self.name = name
        self.args = args if args is not None else []
        self.attrs = attrs if attrs is not None else []
        self.children = children if children is not None else []


class MumlParseError(ValueError):
    def __init__(self, line: int, message: str):
        self.line = line
        self.message = message
        super().__init__(f"{line}: {message}")


class _Parser:
    def __init__(self, data: str):
        self.data = data
        self.pos = 0
        self.line = 1

    def at_end(self) -> bool:
        return self.pos >= len(self.data)

    def peek(self) -> str | None:
        if self.at_end():
            return None
        return self.data[self.pos]

    def raise_error(self, message: str):
        raise MumlParseError(self.line, message)

    def skip_ws(self):
        while not self.at_end():
            c = self.data[self.pos]
            if c == "#":
                while not self.at_end() and self.data[self.pos] != "\n":
                    self.pos += 1
            elif _is_space(c):
                if c == "\n":
                    self.line += 1
                self.pos += 1
            else:
                break

    def skip_hws(self) -> bool:
        start = self.pos
        while not self.at_end() and _is_hspace(self.data[self.pos]):
            self.pos += 1
        return self.pos != start

    def parse_string(self) -> str:
        c = self.peek()
        if c in ("'", '"'):
            quote = c
            self.pos += 1
            start = self.pos
            while not self.at_end() and self.data[self.pos] != quote:
                if self.data[self.pos] == "\n":
                    self.line += 1
                self.pos += 1
            if self.at_end():
                self.raise_error("unterminated string")
            value = self.data[start : self.pos]
            self.pos += 1
            return value

        start = self.pos
        while not self.at_end() and not _is_special(self.data[self.pos]):
            self.pos += 1
        if self.pos == start:
            self.raise_error("expected string")
        return self.data[start : self.pos]

    def parse_node(self) -> Node:
        node = Node(self.parse_string())

        while not self.at_end():
            skipped = self.skip_hws()
            c = self.peek()
            if c is None or not _can_start_string(c):
                break
            if not skipped:
                self.raise_error("expected whitespace")

            value = self.parse_string()
            if self.peek() == "=":
                self.pos += 1
                node.attrs.append((value, self.parse_string()))
            else:
                node.args.append(value)

        if self.peek() == "{":
            self.pos += 1
            self.parse_children(node)
            self.skip_ws()
            if self.peek() != "}":
                self.raise_error("expected '}'")
            self.pos += 1

        return node

    def parse_children(self, parent: Node):
        while True:
            self.skip_ws()
            c = self.peek()
            if c is None or c == "}":
                return
            parent.children.append(self.parse_node())


def _is_hspace(c: str) -> bool:
    return c == " " or c == "\t"


def _is_space(c: str) -> bool:
    return c == " " or c == "\t" or c == "\n" or c == "\r"


def _is_special(c: str) -> bool:
    return (
        _is_space(c)
        or c == "#"
        or c == "="
        or c == "{"
        or c == "}"
        or c == "'"
        or c == '"'
    )


def _can_start_string(c: str) -> bool:
    return c == "'" or c == '"' or not _is_special(c)


def loads(s: str) -> Node:
    parser = _Parser(s)
    root = Node("")
    parser.parse_children(root)
    parser.skip_ws()
    if not parser.at_end():
        parser.raise_error("expected end of input")
    return root


def load(file) -> Node:
    return loads(file.read())


def loadf(path: str) -> Node:
    with open(path, "r", encoding="utf-8") as f:
        return load(f)


def dumps(root: Node) -> str:
    lines: list[str] = []

    if root.name == "":
        nodes = root.children
    else:
        nodes = [root]

    for node in nodes:
        _dump_node(node, 0, lines)

    return "\n".join(lines)


def dump(root: Node, file):
    file.write(dumps(root))


def dumpf(root: Node, path: str):
    with open(path, "w", encoding="utf-8") as f:
        dump(root, f)


def _dump_node(node: Node, indent: int, lines: list[str]):
    prefix = " " * (indent * 4)
    parts = [_dump_string(node.name)]
    parts.extend(_dump_string(arg) for arg in node.args)
    parts.extend(
        f"{_dump_string(name)}={_dump_string(value)}" for name, value in node.attrs
    )
    head = prefix + " ".join(parts)

    if node.children:
        lines.append(head + " {")
        for child in node.children:
            _dump_node(child, indent + 1, lines)
        lines.append(prefix + "}")
    else:
        lines.append(head)


def _dump_string(value: str) -> str:
    if value == "":
        return '""'
    if all(not _is_special(c) for c in value):
        return value
    if '"' not in value:
        return f'"{value}"'
    if "'" not in value:
        return f"'{value}'"
    raise ValueError(f"cannot represent string with both quote types: {value!r}")


if __name__ == "__main__":
    import argparse
    import pprint

    def print_node(n: Node, indent=0):
        indent_str = indent * 2 * " "
        print(f"{indent_str}NODE {n.name}")
        for a in n.args:
            print(f"{indent_str}'{a}'")
        for k, v in n.attrs:
            print(f"{indent_str}'{k}' = '{v}'")
        for c in n.children:
            print_node(c, indent + 1)

    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    args = parser.parse_args()

    root = loadf(args.input)
    for n in root.children:
        print_node(n)
