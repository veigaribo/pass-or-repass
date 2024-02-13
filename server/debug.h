#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define Debug(Prefix, Format, ...)                                             \
  printf("[" #Prefix "] " Format __VA_OPT__(, ) __VA_ARGS__)
#else
#define Debug(Prefix, Format, ...)
#endif

#endif /* DEBUG_H */
