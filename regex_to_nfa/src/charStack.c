#include "charStack.h"

void init_stack(CharStack* s) {
    s->top = -1;
}

int push(CharStack* s, char value) {
    if (s->top >= MAX_STACK - 1) return 0;
    s->data[++(s->top)] = value;
    return 1;
}

char pop(CharStack* s) {
    if (s->top < 0) return '\0';
    return s->data[(s->top)--];
}

char peek(CharStack* s) {
    if (s->top < 0) return '\0';
    return s->data[s->top];
}

int is_empty(CharStack* s) {
    return s->top == -1;
}