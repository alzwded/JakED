#error "this is not a real source file"
#ifndef CPRINTF_H
#define CPRINTF_H

>>>GUARDS<<<

enum class CPK : int
{
>>>KEYS<<<
    LAST
};

constexpr int EnabledKeys[] = {
>>>MACROS<<<
};

constexpr const char* KeyTranslation[] = {
>>>TRANSLATIONS<<<
};

static_assert(sizeof(EnabledKeys)/sizeof(EnabledKeys[0]) == static_cast<int>(CPK::LAST), "Missing enabled key entry");

template<CPK key, int N, typename... Args>
inline void cprintf(const char (&format)[N], Args... args)
{
    if constexpr(EnabledKeys[static_cast<int>(key)]) {
        char format1[N + 4];
        memcpy(format1, "%s: ", 4);
        memcpy(format1 + 4, format, N);
        format1[N + 4 - 1] = '\0';
        //printf("!!!format is %s!!!!\n", format1); // he he
        (void) fprintf(stderr, format1, KeyTranslation[static_cast<int>(key)], args...);
    } else {
    }
}
#endif
