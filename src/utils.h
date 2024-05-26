#ifndef __FFNES_UTILS_H__
#define __FFNES_UTILS_H__

#ifndef offsetof
#define offsetof(type, member)          ((int)&((type*)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) (type*)((char*)(ptr) - offsetof(type, member))
#endif

#endif
