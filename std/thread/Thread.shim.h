/* pthread mutex/cond and an anonymous semaphore. License: MIT (see LICENSE). */
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef CT_STD_THREAD_SHIM_H
#define CT_STD_THREAD_SHIM_H
#include <pthread.h>
#include <stdlib.h>

static inline void *std_thread_mutex_new(void)
{
    pthread_mutex_t *m = (pthread_mutex_t *)malloc(sizeof *m);
    if (!m) return 0;
    if (pthread_mutex_init(m, 0) != 0) { free(m); return 0; }
    return m;
}
static inline int std_thread_mutex_lock(void *m) { return m && pthread_mutex_lock((pthread_mutex_t *)m) == 0 ? 0 : -1; }
static inline int std_thread_mutex_unlock(void *m) { return m && pthread_mutex_unlock((pthread_mutex_t *)m) == 0 ? 0 : -1; }
static inline int std_thread_mutex_trylock(void *m) { return m && pthread_mutex_trylock((pthread_mutex_t *)m) == 0 ? 0 : -1; }
static inline void std_thread_mutex_free(void *m)
{
    if (!m) return;
    pthread_mutex_destroy((pthread_mutex_t *)m);
    free(m);
}

static inline void *std_thread_cond_new(void)
{
    pthread_cond_t *c = (pthread_cond_t *)malloc(sizeof *c);
    if (!c) return 0;
    if (pthread_cond_init(c, 0) != 0) { free(c); return 0; }
    return c;
}
static inline int std_thread_cond_wait(void *c, void *m) { return c && m && pthread_cond_wait((pthread_cond_t *)c, (pthread_mutex_t *)m) == 0 ? 0 : -1; }
static inline int std_thread_cond_signal(void *c) { return c && pthread_cond_signal((pthread_cond_t *)c) == 0 ? 0 : -1; }
static inline int std_thread_cond_broadcast(void *c) { return c && pthread_cond_broadcast((pthread_cond_t *)c) == 0 ? 0 : -1; }
static inline void std_thread_cond_free(void *c)
{
    if (!c) return;
    pthread_cond_destroy((pthread_cond_t *)c);
    free(c);
}

struct std_thread_sem { pthread_mutex_t mutex; pthread_cond_t cond; int count; };
static inline void *std_thread_sem_new(int value)
{
    struct std_thread_sem *s;
    if (value < 0) return 0;
    s = (struct std_thread_sem *)malloc(sizeof *s);
    if (!s) return 0;
    pthread_mutex_init(&s->mutex, 0);
    pthread_cond_init(&s->cond, 0);
    s->count = value;
    return s;
}
static inline int std_thread_sem_wait(void *handle)
{
    struct std_thread_sem *s = (struct std_thread_sem *)handle;
    if (!s) return -1;
    pthread_mutex_lock(&s->mutex);
    while (s->count == 0) pthread_cond_wait(&s->cond, &s->mutex);
    s->count--;
    pthread_mutex_unlock(&s->mutex);
    return 0;
}
static inline int std_thread_sem_post(void *handle)
{
    struct std_thread_sem *s = (struct std_thread_sem *)handle;
    if (!s) return -1;
    pthread_mutex_lock(&s->mutex);
    s->count++;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mutex);
    return 0;
}
static inline void std_thread_sem_free(void *handle)
{
    struct std_thread_sem *s = (struct std_thread_sem *)handle;
    if (!s) return;
    pthread_mutex_destroy(&s->mutex);
    pthread_cond_destroy(&s->cond);
    free(s);
}
struct std_thread_handle { pthread_t id; };
static inline void *std_thread_create(void *start, void *arg)
{
    struct std_thread_handle *h;
    if (!start) return 0;
    h = (struct std_thread_handle *)malloc(sizeof *h);
    if (!h) return 0;
    if (pthread_create(&h->id, 0, (void *(*)(void *))start, arg) != 0) { free(h); return 0; }
    return h;
}
static inline int std_thread_join(void *handle)
{
    struct std_thread_handle *h = (struct std_thread_handle *)handle;
    if (!h) return -1;
    pthread_join(h->id, 0);
    free(h);
    return 0;
}


#endif
