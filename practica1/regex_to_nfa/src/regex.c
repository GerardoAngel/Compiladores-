#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "charStack.h"
#include "regex.h"

int precedence (char op) {
    switch (op) {
        case '*': return 5;
        case '+': return 4;
        case '?': return 3;
        case '.': return 2;
        case '|': return 1;
        case '(' : return -1;
        case ')' : return -2;
        default: return 0;
    }
}

static void insert_concat(char *pattern) {

    char result[MAX_REGEX];
    int j = 0;

    for (int i = 0; pattern[i] != '\0'; i++) {

        result[j++] = pattern[i];

        if (pattern[i] == '(' || pattern[i] == '|')
            continue;

        if (pattern[i + 1] != '\0' &&
            pattern[i + 1] != ')' &&
            pattern[i + 1] != '|' &&
            pattern[i + 1] != '*' &&
            pattern[i + 1] != '+' &&
            pattern[i + 1] != '?'){
            result[j++] = '.';
        }
    }

    result[j] = '\0';
    strcpy(pattern, result);
}

static char* infix_to_postfix(char *concat){

    char *result = malloc(MAX_REGEX);
    if (!result) return NULL;
 
    CharStack s;
    init_stack(&s);

    int j = 0;
    int i = 0;

    while (concat[i] != '\0') {

        char c = concat[i++];

        if (precedence(c) == 0) {
            result[j++] = c;
        }
        else {
            if (c == '(') {
                push(&s, c);
            }
            else if (c == ')') {
                while (!is_empty(&s) && peek(&s) != '(')
                    result[j++] = pop(&s);
                pop(&s);
            }
            else {
                while (!is_empty(&s) &&
                       peek(&s) != '(' &&
                       precedence(peek(&s)) >= precedence(c))
                {
                    result[j++] = pop(&s);
                }
                push(&s, c);
            }
        }
    }

    while (!is_empty(&s))
        result[j++] = pop(&s);

    result[j] = '\0';
    return result;
}   

regex parse_regex(const char* pattern) {

    regex r;
    r.size = 0;

    char buffer[MAX_REGEX];
    strcpy(buffer, pattern);

    insert_concat(buffer);

    char* postfix = infix_to_postfix(buffer);

    if (!postfix) {
        fprintf(stderr, "Error converting regex\n");
        return r;
    }

    for (int i = 0; postfix[i] != '\0'; i++) {
        r.items[r.size++].value = postfix[i];
    }

    free(postfix);
    return r;
}