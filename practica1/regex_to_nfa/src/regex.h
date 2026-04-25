#ifndef REGEX_H
#define REGEX_H

#define MAX_REGEX 1024

typedef struct {
    char value;
} regex_item;

typedef struct {
    regex_item items[MAX_REGEX];
    int size;
} regex;

regex parse_regex(const char* pattern);

#endif