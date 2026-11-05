#include "colours.h"

struct colour_ll_s{
    uint index;
    uchar r;
    uchar g;
    uchar b;

    struct colour_ll_s *next;
};

struct colour_ll_s *colour_ll;
struct colour_ll_s *colour_ll_end;

uint solid_colour_try(uchar r, uchar g, uchar b){

    // Reuse the colour if we've already made it
    for (struct colour_ll_s *c = colour_ll; c; c = c->next){
        if (c->r == r && c->g == g && c->b == b) return c->index;
    }

    struct colour_ll_s *new = mem_malloc(sizeof(struct colour_ll_s));
    new->index = solid_colour_add(r, g, b);
    new->r = r;
    new->g = g;
    new->b = b;

    if (!colour_ll) colour_ll = new;
    else colour_ll_end->next = new;
    colour_ll_end = new;

    return new->index;
}
