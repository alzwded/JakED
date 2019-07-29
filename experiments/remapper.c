/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//#define WIN32_LEAN_AND_MEAN
//#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <memoryapi.h>
#include <io.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SIZE_T GetNextIncrement(SIZE_T qwLargePageMinimum, DWORD step) {
    SIZE_T qwNewSize = qwLargePageMinimum;
    while(--step && qwNewSize < (1 << 27)) {
        qwNewSize = qwNewSize << 1;
    }
    if(step > 0) return qwNewSize * step;
    else return qwNewSize;
}

#define EXPERIMENT 2
#define CRASH_AT_SOME_POINT 0

/*
EXPERIMENT == 1
    In this case, it always creates a view for each individual write and each individual read.
    The results were attrocious, having the same performance (or worse?) than fread/fwrite.
    I didn't time it because it took like 10 minutes, while experiment 2 took like 5 seconds.

EXPERIMENT == 2
    In this case, the file view is kept around while the file mapping is valid. The file view
    is "lazy initialized". It FlushViewOfFile'es every 8MB to simulate "transactions". We could
    use this transaction approach in the main program since a transaction would be one command or so.
    This can be later tuned to hit the sweet spot. FlushViewOfFile (IFF I read the docs right)
    only writes dirty pages (where a page has an arbitrary OS specific size, i.e. with large memory
    chunks, it only writes the most recent additions). Allegedly, FlushViewOfFile does not
    update file metadata, which requires FlushFileBuffers on the file itself. FlushFileBuffers
    claims it's inefficient. I'm not sure what this "file metadata" actually is. As long as the
    data is recoverable, I don't care about metadata.

CRASH_AT_SOME_POINT == 1
    Cause an abort() crash to test if memory got committed. It seems it is.
*/

#if EXPERIMENT == 1
#elif EXPERIMENT == 2
unsigned char *mem = NULL;
#endif

__int64 Read(HANDLE h, SIZE_T written, SIZE_T size, SIZE_T addr)
{
#if EXPERIMENT == 1
    unsigned char* mem = (unsigned char*)MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
#elif EXPERIMENT == 2
    if(!mem) mem = (unsigned char*)MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
#endif

    __int64 rval = 0;
    memcpy(&rval, mem + addr, sizeof(__int64));

#if EXPERIMENT == 1
    UnmapViewOfFile(mem);
#elif EXPERIMENT == 2
#endif

    return rval;
}

void Write(HANDLE* h, SIZE_T* written, __int64 i, SIZE_T* size, SIZE_T qwLargePageMinimum, SIZE_T* increment, HANDLE hFile)
{
    if(*written + sizeof(__int64) >= *size) {
#if EXPERIMENT == 1
#elif EXPERIMENT == 2
        UnmapViewOfFile(mem);
        mem = NULL;
#endif
        CloseHandle(*h);
        ++(*increment);
        *size = GetNextIncrement(qwLargePageMinimum, *increment);

        *h = CreateFileMapping(hFile, NULL, PAGE_READWRITE, ((*size) >> 32) & 0xFFFFFFFF, (*size) & 0xFFFFFFFF, NULL);
        if(!*h || *h == INVALID_HANDLE_VALUE) __debugbreak();
    }
#if EXPERIMENT == 1
    unsigned char* mem = (unsigned char*)MapViewOfFile(*h, FILE_MAP_ALL_ACCESS, 0, 0, *size);
#elif EXPERIMENT == 2
    if(!mem) mem = (unsigned char*)MapViewOfFile(*h, FILE_MAP_ALL_ACCESS, 0, 0, *size);
#endif

    memcpy(mem + *written, &i, sizeof(__int64));
    (*written) += sizeof(__int64);

#if EXPERIMENT == 1
    UnmapViewOfFile(mem);
#elif EXPERIMENT == 2
#endif
}

int main(void) {
    //FILE* f = tmpfile();
    FILE* f = fopen("mem.tmp", "w+");
    HANDLE h = (HANDLE)(_get_osfhandle(_fileno(f)));
    SIZE_T qwLargePageMinimum = 32;//1024 * 64;
    SIZE_T increment = 1;

    SIZE_T size = GetNextIncrement(qwLargePageMinimum, increment);
    HANDLE fileMapping = CreateFileMapping(h, NULL, PAGE_READWRITE, (size >> 32) & 0xFFFFFFFF, size & 0xFFFFFFFF, NULL);
    SIZE_T written = 0;

    __int64 i, j;
    unsigned sum = 0, chk = 0;
    unsigned sum1 = 0, chk1 = 0;

#define SIZE (__int64)50000000
//                      '  '

    for(i = 0; i < SIZE; ++i) {
        if(i % (1024 * 1024) == 0) {
#if EXPERIMENT == 1
#elif EXPERIMENT == 2
            FlushViewOfFile(mem, size);
#endif
            printf("i = %I64d\n", i / (1024 * 1024));
        }
        sum += (unsigned)(i & 0xFFFFFFFFu);
        chk = chk << 1;
        chk ^= (unsigned)((i >> 8) & 0xFFFFFFFFu);
        Write(&fileMapping, &written, i, &size, qwLargePageMinimum, &increment, h);
#if CRASH_AT_SOME_POINT
        if( i / (1024 * 1024) == 31
        && (i % (1024 * 1024) == 128)
        )
            abort();
#endif
    }

#if EXPERIMENT == 1
#elif EXPERIMENT == 2
    //FlushViewOfFile(mem, size);
    UnmapViewOfFile(mem);
    mem = NULL;
#endif

    for(j = 0; j < SIZE; ++j) {
        if(j % (1024 * 1024) == 0) printf("j = %I64d\n", j / (1024 * 1024));
        SIZE_T addr = j * 8;
        __int64 i = Read(fileMapping, written, size, addr);
        sum1 += (unsigned)(i & 0xFFFFFFFFu);
        chk1 = chk1 << 1;
        chk1 ^= (unsigned)((i >> 8) & 0xFFFFFFFFu);
    }

    printf("sum: %u %u\nchk: %u %u\n", sum, sum1, chk, chk1);

#if EXPERIMENT == 1
#elif EXPERIMENT == 2
    UnmapViewOfFile(mem);
#endif

    CloseHandle(fileMapping);
    fclose(f);
}
