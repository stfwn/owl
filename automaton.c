#include "automaton.h"

#include "state-array.h"

#include <stdio.h>
#include <stdlib.h>

static void invalidate_epsilon_closure(struct automaton *a)
{
    if (a->epsilon_closure_for_state == 0)
        return;
    for (uint32_t i = 0; i < a->number_of_states; ++i) {
        struct epsilon_closure *closure = &a->epsilon_closure_for_state[i];
        state_array_destroy(&closure->reachable);
        free(closure->action_indexes);
        free(closure->actions);
    }
    free(a->epsilon_closure_for_state);
    a->epsilon_closure_for_state = 0;
}

static void state_add_transition(struct state *s, state_id target,
 symbol_id symbol, uint16_t action)
{
    if (s->number_of_transitions == 0xffff) {
        fprintf(stderr, "error: too many transitions for a single state.\n");
        exit(-1);
    }
    uint16_t id = s->number_of_transitions++;
    s->transitions = grow_array(s->transitions, &s->transitions_allocated_bytes,
     (uint32_t)s->number_of_transitions * sizeof(struct transition));
    s->transitions[id] = (struct transition){
        .target = target,
        .symbol = symbol,
        .action = action,
    };
}

void automaton_add_transition(struct automaton *a, state_id source,
 state_id target, symbol_id symbol)
{
    automaton_add_transition_with_action(a, source, target, symbol, 0);
}

static void grow_states(struct automaton *a, uint32_t state)
{
    if (state < a->number_of_states)
        return;
    a->number_of_states = (uint32_t)state + 1;
    a->states = grow_array(a->states, &a->states_allocated_bytes,
     a->number_of_states * sizeof(struct state));
}

void automaton_add_transition_with_action(struct automaton *a, state_id source,
 state_id target, symbol_id symbol, uint16_t action)
{
    invalidate_epsilon_closure(a);
    state_id max_state = source;
    if (target > source)
        max_state = target;
    grow_states(a, max_state);
    if (symbol != SYMBOL_EPSILON && symbol >= a->number_of_symbols)
        a->number_of_symbols = symbol + 1;
    state_add_transition(&a->states[source], target, symbol, action);
}

void automaton_set_start_state(struct automaton *a, state_id state)
{
    grow_states(a, state);
    a->start_state = state;
}

void automaton_mark_accepting_state(struct automaton *a, state_id state)
{
    grow_states(a, state);
    a->states[state].accepting = true;
}

void automaton_reverse(struct automaton *a, struct automaton *reversed)
{
    state_id start_state = a->number_of_states;
    for (uint32_t i = 0; i < a->number_of_states; ++i) {
        struct state *s = &a->states[i];
        for (uint32_t j = 0; j < s->number_of_transitions; ++j) {
            struct transition *t = &s->transitions[j];
            automaton_add_transition(reversed, t->target, i, t->symbol);
        }
        if (s->accepting)
            automaton_add_transition(reversed, start_state, i, SYMBOL_EPSILON);
    }
    automaton_set_start_state(reversed, start_state);
    automaton_mark_accepting_state(reversed, a->start_state);
}

void automaton_print(struct automaton *a)
{
    printf("start = %u\n", a->start_state);
    for (uint32_t i = 0; i < a->number_of_states; ++i) {
        struct state *s = &a->states[i];
        if (s->accepting) {
            if (s->transition_symbol)
                printf("%4u   -- accept --> %x\n", i, s->transition_symbol);
            else
                printf("%4u   -- accept -->\n", i);
        }
        for (uint32_t j = 0; j < s->number_of_transitions; ++j) {
            struct transition *t = &s->transitions[j];
            if (t->symbol == SYMBOL_EPSILON)
                printf("%4u   ---------->%4u", i, t->target);
            else
                printf("%4u   - %4x   ->%4u", i, t->symbol, t->target);
            if (t->action != 0)
                printf(" (%x)\n", t->action);
            else
                printf("\n");
        }
    }
}

void automaton_move(struct automaton *from, struct automaton *to)
{
    *to = *from;
    memset(from, 0, sizeof(struct automaton));
}

void automaton_clear(struct automaton *a)
{
    invalidate_epsilon_closure(a);
    for (uint32_t i = 0; i < a->number_of_states; ++i)
        free(a->states[i].transitions);
    memset(a->states, 0, a->states_allocated_bytes);
    a->number_of_states = 0;
    a->number_of_symbols = 0;
    a->start_state = 0;
}

void automaton_destroy(struct automaton *a)
{
    automaton_clear(a);
    free(a->states);
}
