/*============================================================================
 * util/ringbuf.h — Lightweight fixed-size ring buffer (header-only)
 *
 * Usage:
 *   RINGBUF_DEF(my_rb, 128);          // define a 128-byte ring buffer
 *   RINGBUF_PUSH(my_rb, 0x42);        // push one byte (silently drops if full)
 *   if (!RINGBUF_EMPTY(my_rb)) {
 *       uint8_t b;
 *       RINGBUF_POP(my_rb, b);
 *   }
 *
 * SIZE must be a power of 2 so that masking works.
 *===========================================================================*/
#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>

#define RINGBUF_DEF(name, size)                                 \
    static struct {                                             \
        uint8_t  buf[(size)];                                   \
        volatile uint16_t head;                                 \
        volatile uint16_t tail;                                 \
    } name = { {0}, 0, 0 }

#define RINGBUF_MASK_(name)   ((uint16_t)(sizeof((name).buf) - 1u))

#define RINGBUF_PUSH(name, byte) do {                           \
    uint16_t nxt_ = ((name).head + 1u) & RINGBUF_MASK_(name);  \
    if (nxt_ != (name).tail) {                                  \
        (name).buf[(name).head] = (byte);                       \
        (name).head = nxt_;                                     \
    }                                                           \
} while (0)

#define RINGBUF_POP(name, out) do {                             \
    (out) = (name).buf[(name).tail];                            \
    (name).tail = ((name).tail + 1u) & RINGBUF_MASK_(name);    \
} while (0)

#define RINGBUF_EMPTY(name)     ((name).head == (name).tail)

#define RINGBUF_FULL(name)      \
    ((((name).head + 1u) & RINGBUF_MASK_(name)) == (name).tail)

#define RINGBUF_COUNT(name)     \
    (((name).head - (name).tail) & RINGBUF_MASK_(name))

#define RINGBUF_CLEAR(name) do {                                \
    (name).head = 0; (name).tail = 0;                           \
} while (0)

#endif /* RINGBUF_H */
