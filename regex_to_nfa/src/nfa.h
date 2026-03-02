#ifndef NFA_H
#define NFA_H
#include "regex.h"

#define EPSILON '\0'
#define MAX_stateS 512

typedef struct state state;
typedef struct transition transition;


/* ---------- Transiciones ---------- */
struct transition {
    char symbol;            // símbolo o EPSILON
    state* to;              // estado destino
    transition* next;       // siguiente transición
};

/* ---------- Estado ---------- */
struct state {
    int id;
    int is_accepting;
    transition* transitions;
};

/* ---------- Fragmento para Thompson ---------- */
typedef struct {
    state* start;
    state* accept;
} Fragment;

/* ---------- NFA ---------- */
typedef struct {
    state* start;
    state* accept;
    state* states[MAX_stateS];
    int state_count;
} nfa;

/* ---------- API ---------- */

nfa regex_to_nfa(regex r);
int match_nfa(nfa n, const char* str, int len);
void free_nfa(nfa* n);

#endif