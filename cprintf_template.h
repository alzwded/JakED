#error "this is not a real source file"
/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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
