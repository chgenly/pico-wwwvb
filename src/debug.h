#define DEBUG_LEVEL 1

#if DEBUG_LEVEL >= 1
#define dprintf1(...) printf(__VA_ARGS__)
#else
#define dprintf1(...)
#endif

#if DEBUG_LEVEL >= 2
#define dprintf2(...) printf(__VA_ARGS__)
#else
#define dprintf2(...)
#endif

#if DEBUG_LEVEL >= 3
#define dprintf3(...) printf(__VA_ARGS__)
#else
#define dprintf3(...)
#endif

