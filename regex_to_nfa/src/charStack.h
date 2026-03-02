#ifndef STACK_H
#define STACK_H

#define MAX_STACK 256

typedef struct {
    char data[MAX_STACK];
    int top;
} CharStack;

void init_stack(CharStack* s);
int push(CharStack* s, char value);
char pop(CharStack* s);
char peek(CharStack* s);
int is_empty(CharStack* s);

#endif