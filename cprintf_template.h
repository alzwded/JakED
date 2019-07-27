#error "this is not a real source file"
#ifndef CPRINTF_H
#define CPRINTF_H

#include <sal.h>

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


#if 0
// sadly, char (&)[] doesn't work with SAL _Printf_... for some reason
template<CPK key, int N, typename... Args>
inline void __cprintf(_In_z_ _Printf_format_string_params_(1) const char (&format)[N], Args... args)
#endif
template<CPK key, typename... Args>
inline void cprintf(_In_z_ _Printf_format_string_params_(1) const char* const format, Args... args)
{
    if constexpr(EnabledKeys[static_cast<int>(key)]) {
#pragma warning(push)
#pragma warning(disable: 6386 6387 6011)
        auto N = strlen(format);
        char* format1 = (char*)malloc(N + 4 + 1);
        memcpy(format1, "%s: ", 4);
        memcpy(format1 + 4, format, N);
        format1[N + 4] = '\0';
        //printf("!!!format is %s!!!!\n", format1); // he he
        (void) fprintf(stderr, format1, KeyTranslation[static_cast<int>(key)], args...);
        free(format1);
#pragma warning(pop)
    } else {
    }
}
#endif
