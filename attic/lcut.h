/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */

void l_cut(struct list *first, struct list *last, struct list *out)
{
   struct list *before, *after;

   Assert(first);
   Assert(last);
   Assert(out);

   before = first->prev;
   after  = last->next;

   out->next = first;
   out->prev = last;

   before->next = after;
   after->prev  = before;

   first->prev  = out;
   last->next   = out;
}
