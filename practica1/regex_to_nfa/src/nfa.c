#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nfa.h"
#include "regex.h"

static state* create_state(nfa* n){

    state* s = malloc(sizeof(state));

    s->id = n->state_count;
    s->is_accepting = 0;
    s->transitions = NULL;

    n->states[n->state_count] = s;
    n->state_count++;

    return s;
}

static void add_transition(state* from, char symbol, state* to) {
    transition* t = malloc(sizeof(transition));
    t->symbol = symbol;
    t->to = to;
    t->next = from->transitions;
    from->transitions = t;
}

nfa regex_to_nfa(regex r) {
    nfa n;
    n.state_count = 0;
    n.start = NULL;
    n.accept = NULL;


    for (int i = 0; i < 256; i++)
        n.states[i] = NULL;

    Fragment stack[512];
    int top = -1;

    for (int i = 0; i < r.size; i++) {

        char c = r.items[i].value;

        if (c != '.' && c != '|' && c != '*' && c != '+' && c != '?') {

            // símbolo normal
            state* s1 = create_state(&n);
            state* s2 = create_state(&n);
            add_transition(s1, c, s2);

            Fragment f = {s1, s2};
            stack[++top] = f;
        }

        else if (c == '.') {
            Fragment f2 = stack[top--];
            Fragment f1 = stack[top--];

            add_transition(f1.accept, EPSILON, f2.start);

            Fragment f = {f1.start, f2.accept};
            stack[++top] = f;
        }

        else if (c == '|') {
            Fragment f2 = stack[top--];
            Fragment f1 = stack[top--];

            state* start = create_state(&n);
            state* accept = create_state(&n);

            add_transition(start, EPSILON, f1.start);
            add_transition(start, EPSILON, f2.start);

            add_transition(f1.accept, EPSILON, accept);
            add_transition(f2.accept, EPSILON, accept);

            Fragment f = {start, accept};
            stack[++top] = f;
        }

        else if (c == '*') {
            Fragment f1 = stack[top--];

            state* start = create_state(&n);
            state* accept = create_state(&n);

            add_transition(start, EPSILON, f1.start);
            add_transition(start, EPSILON, accept);

            add_transition(f1.accept, EPSILON, f1.start);
            add_transition(f1.accept, EPSILON, accept);

            Fragment f = {start, accept};
            stack[++top] = f;
        }

        else if (c == '+') {
            Fragment f1 = stack[top--];

            state* start = create_state(&n);
            state* accept = create_state(&n);

            add_transition(start, EPSILON, f1.start);
            add_transition(f1.accept, EPSILON, f1.start);
            add_transition(f1.accept, EPSILON, accept);

            Fragment f = {start, accept};
            stack[++top] = f;
        }

        else if (c == '?') {
            Fragment f1 = stack[top--];

            state* start = create_state(&n);
            state* accept = create_state(&n);

            add_transition(start, EPSILON, f1.start);
            add_transition(start, EPSILON, accept);
            add_transition(f1.accept, EPSILON, accept);

            Fragment f = {start, accept};
            stack[++top] = f;
        }
    }

    Fragment result = stack[top];

    for (int i = 0; i < n.state_count; i++)
    n.states[i]->is_accepting = 0;
    result.accept->is_accepting = 1;

    n.start = result.start;
    n.accept = result.accept;

    return n;
}

static void epsilon_closure(state* s, int* visited) {
    if (visited[s->id]) return;
    visited[s->id] = 1;

    for (transition* t = s->transitions; t; t = t->next) {
        if (t->symbol == EPSILON) {
            epsilon_closure(t->to, visited);
        }
    }
}


int match_nfa(nfa n, const char* str, int len) {

    int current[MAX_stateS] = {0};
    int next[MAX_stateS] = {0};

    // Inicializar con estado inicial
    current[n.start->id] = 1;

    // Expandir ε-closure inicial
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < n.state_count; i++) {
            if (!current[i]) continue;

            state* s = n.states[i];
            for (transition* t = s->transitions; t; t = t->next) {
                if (t->symbol == EPSILON && !current[t->to->id]) {
                    current[t->to->id] = 1;
                    changed = 1;
                }
            }
        }
    }

    // Procesar cada símbolo
    for (int c = 0; c < len; c++) {

        memset(next, 0, sizeof(next));

        for (int i = 0; i < n.state_count; i++) {

            if (!current[i]) continue;

            state* s = n.states[i];

            for (transition* t = s->transitions; t; t = t->next) {
                if (t->symbol == str[c]) {
                    next[t->to->id] = 1;
                }
            }
        }

        // Expandir ε-closure del nuevo conjunto
        changed = 1;
        while (changed) {
            changed = 0;
            for (int i = 0; i < n.state_count; i++) {
                if (!next[i]) continue;

                state* s = n.states[i];
                for (transition* t = s->transitions; t; t = t->next) {
                    if (t->symbol == EPSILON && !next[t->to->id]) {
                        next[t->to->id] = 1;
                        changed = 1;
                    }
                }
            }
        }

        memcpy(current, next, sizeof(current));
    }

    return current[n.accept->id];
}

void free_nfa(nfa* n) {

    for (int i = 0; i < n->state_count; i++) {
        transition* t = n->states[i]->transitions;
        while (t) {
            transition* tmp = t;
            t = t->next;
            free(tmp);
        }
        free(n->states[i]);
    }
}