
#ifndef _ATOMIC_H_
#define _ATOMIC_H_


#if MAX_THREADS > 1

#define CASv(var, old, new) __sync_bool_compare_and_swap(&(var), old, new)
#define CASp(ptr, old, new) __sync_bool_compare_and_swap(ptr, old, new)
#define INCR(var) __sync_add_and_fetch(&(var), 1)

#else

#define CASv(var, old, new) if((var) == (old)) { (var) = (new); }
#define CASp(ptr, old, new) if(*(ptr) == (old)) { *(ptr) = (new); }
#define INCR(var) (++(var))

#endif


#endif

