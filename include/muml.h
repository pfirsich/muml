#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* data;
    size_t size;
} muml_string;

typedef struct {
    muml_string name;
    muml_string value;
} muml_attr;

typedef struct muml_node {
    muml_string name;
    muml_string* args;
    size_t arg_count;
    muml_attr* attrs;
    size_t attr_count;
    muml_node* first_child;
    muml_node* next_sibling;
} muml_node;

typedef struct {
    const char* cursor;
    uint32_t error_line;
    const char* error_message;
} muml_parse_result;

// nodes[0] is the root node. children of nodes[0] are top-level nodes.
muml_parse_result muml_parse(const char* data, size_t size, muml_node* nodes, size_t max_node_count,
    muml_string* args, size_t max_arg_count, muml_attr* attrs, size_t max_attr_count);

// Helpers

// p must point into source[0..size]
uint32_t muml_get_line(const char* source, size_t size, const char* p);

// returns NULL if node is NULL or no child with that name exists
const muml_node* muml_child(const muml_node* node, const char* name);

// return NULL if node is NULL or index out of range
const muml_string* muml_get_arg(const muml_node* node, uint32_t index);
const muml_string* muml_get_attr(const muml_node* node, const char* name);

// s and out must not be NULL
bool muml_parse_float(const muml_string* s, float* out);
bool muml_parse_int(const muml_string* s, int64_t* out); // just decimal
bool muml_parse_bool(const muml_string* s, bool* out); // just "true" and "false"

// return fallback if s is NULL
float muml_float(const muml_string* s, float fallback);
int64_t muml_int(const muml_string* s, int64_t fallback);
bool muml_bool(const muml_string* s, bool fallback);
muml_string muml_str(const muml_string* s, muml_string fallback);

// Parses num floats from args. Intended for vectors. returns true on success.
bool muml_float_n(const muml_node*, float* values, size_t num);
bool muml_int_n(const muml_node*, int64_t* values, size_t num);

#ifdef __cplusplus
}
#endif
