#ifndef QUE_H
#define QUE_H

#define Que_push(name, val) \
name[name ## _cursor] = val; \
name ## _cursor++; \
if (name ## _cursor == name ## _len) { \
    name ## _cursor = 0;\
}

#define Que_get(name, idx) name[(name ## _cursor + idx) % name ## _len]
#endif