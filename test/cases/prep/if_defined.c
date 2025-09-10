#if defined MY_DEFINE
void some();
#endif

#define SOME1
#define SOME2

#if defined SOME1 && defined SOME2
void actual();
#endif