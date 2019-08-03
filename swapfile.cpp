/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "swapfile.h"

#include <vector>
#include <list>
#include <cstdio>
#include <string>
#include <sstream>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>

#include "cprintf.h"

class FileImpl;
class FileLine : public ILine
{
    friend class FileImpl;
    FILE* const m_file;
    // technically, this is not supported; we'll regroup if/when these asserts fail
    static_assert(std::is_convertible<fpos_t, int64_t>::value, "fpos_t is not convertible to int64_t");
    static_assert(std::is_convertible<int64_t, fpos_t>::value, "fpos_t is not convertible to int64_t");
    static_assert(sizeof(fpos_t) <= sizeof(int64_t), "fpos_t is not convertible to int64_t");
    int64_t m_pos;
public:
#   pragma pack(push, 1)
    // I don't like counting, this let's me use offsetof()
    struct LineFormat
    {
        int64_t next;
        uint16_t sz;
        char* text;
    };
#   pragma pack(pop)

    bool operator==(ILine const& other) const
    {
        auto pp = dynamic_cast<FileLine const*>(&other);
        if(!pp) return false;
        cprintf<CPK::swap>("FileLine== %zd %zd\n", m_pos, pp->m_pos);
        if(m_file != pp->m_file) return false;
        if(m_pos != pp->m_pos) return false;
        return true;
    }

    FileLine(FILE* file, fpos_t pos)
        : m_file(file)
        , m_pos(pos)
    {}

    size_t length() override
    {
        fpos_t offset = m_pos + offsetof(LineFormat, sz);
        fsetpos(m_file, &offset);
        uint16_t len = 0;
        fread(&len, 2, 1, m_file);
        cprintf<CPK::swap>("[%p] Read length %u from %I64d\n", m_file, len, m_pos);
        return len;
    }

    Text text() override
    {
        fpos_t offset = m_pos + offsetof(LineFormat, sz);
        fsetpos(m_file, &offset);
        uint16_t len = 0;
        fread(&len, 2, 1, m_file);
        if(len == RefMagic) throw std::runtime_error("Invalid indirection");
        char* rval = (char*)malloc(len + 1);
        if(!rval) std::terminate();
        rval[len] = '\0';
        offset = m_pos + offsetof(LineFormat, text);
        fsetpos(m_file, &offset);
        cprintf<CPK::swap>("[%p] Read %u chars from %I64d\n", m_file, len, m_pos);
        fread(rval, 1, len, m_file);
        return Text(len, (uint8_t*)rval, std::function<void(void*)>([](void*p) { free(p); }));
    }

    LinePtr next() override
    {
        fpos_t offset = m_pos + offsetof(LineFormat, next);
        fsetpos(m_file, &offset);
        int64_t nextPos = 0;
        fread(&nextPos, sizeof(int64_t), 1, m_file);
        cprintf<CPK::swap>("[%p] next: %I64d -> %I64d\n", m_file, m_pos, nextPos);
        if(nextPos == 0) return {};
        return std::make_shared<FileLine>(m_file, nextPos);
    }

    void link(LinePtr const& p) override
    {
        auto pp = p.DownCast<FileLine>(); //std::dynamic_pointer_cast<FileLine>(p);
        fpos_t offset = m_pos + offsetof(LineFormat, next);
        fsetpos(m_file, &offset);
        int64_t nextPos = (pp) ? pp->m_pos : 0;
        cprintf<CPK::swap>("[%p] link: %I64d -> %I64d\n", m_file, m_pos, nextPos);
        fwrite(&nextPos, sizeof(int64_t), 1, m_file);
    }

    LinePtr Copy() override
    {
        return std::make_shared<FileLine>(m_file, m_pos);
    }

    Text ref() override
    {
        static_assert(sizeof(fpos_t) <= sizeof(uint8_t*), "fpos_t is smaller than a pointer");
        return Text(RefMagic, (uint8_t*)(m_pos));
    }

    LinePtr deref() override
    {
        fpos_t offset = m_pos + offsetof(LineFormat, sz);
        fsetpos(m_file, &offset);
        uint16_t len = 0;
        fread(&len, 2, 1, m_file);
        if(len != RefMagic) throw std::runtime_error("Invalid indirection");
        offset = m_pos + offsetof(LineFormat, text);
        fsetpos(m_file, &offset);
        cprintf<CPK::swap>("[%p] Read %u chars from %I64d\n", m_file, len, m_pos);
        fpos_t target = 0;
        fread(&target, sizeof(target), 1, m_file);
        return std::make_shared<FileLine>(m_file, target);
    }

}; // FileLine

class FileImpl : public ISwapImpl
{
   // I've heard online that one must fseek(0, CUR) for ftell
   // to work when switching between R&W in update mode; I even
   // saw it written in some man 3 page somewhere. I could not
   // find it in the last publicly published C standard draft,
   // so IDK, based on pure observation, the first statement is
   // correct. Maybe I haven't read the standard fully
    FILE* m_file;
    std::string m_name;
    FileImpl(FILE* f, std::string name) : m_file(f), m_name(name) {}

    static std::string GetTEMPFileName()
    {
        static char tempDir[4096] = {'\0'};
        if(*tempDir == '\0') {
            DWORD length = GetTempPathA(sizeof(tempDir), tempDir);
            if(length <= 0 || length >= 4096) return "";
            tempDir[length] = '\0';
        }
        char lpTempFileName[MAX_PATH + 1] = {'\0'};
        LPCSTR lpPath = tempDir, lpPrefix = "jed";
        DWORD uUnique = 0;
        if(GetTempFileNameA(lpPath, lpPrefix, uUnique, lpTempFileName)) {
            cprintf<CPK::swap>("Computed temp file: %s\n", lpTempFileName);
            return lpTempFileName;
        }
        return "";
    }

    static std::string GetCWDFileName()
    {
        char lpTempFileName[MAX_PATH + 1] = {'\0'};
        LPCSTR lpPath = ".", lpPrefix = "jed";
        DWORD uUnique = 0;
        if(GetTempFileNameA(lpPath, lpPrefix, uUnique, lpTempFileName)) {
            cprintf<CPK::swap>("Computed temp file: %s\n", lpTempFileName);
            return lpTempFileName;
        }
        return "";
    }

public:
#   pragma pack(push, 1)
    // I don't like counting, this let's me use offsetof()
    struct Header
    {
        int64_t head;
        uint16_t padding;
        int64_t cut;
        int64_t undo;
    };
#   pragma pack(pop)

    static ISwapImpl* Create()
    {
        Header head;
        memset(&head, 0, sizeof(head));

        // try tmpfile
        FILE *f = tmpfile();
        if(f) {
            fseek(f, 0, SEEK_SET);
            if(1 == fwrite(&head, sizeof(Header), 1, f)) {
                cprintf<CPK::swap>("Will use tmpfile() [%p]\n", f);
                return new FileImpl(f, "");
            }
        }

        std::string (*tempFunctions[])() = {
            GetTEMPFileName,
            GetCWDFileName,
        };
        for(auto&& tempFunction : tempFunctions) {
            auto name = tempFunction().c_str();
            f = fopen(name, "wb");
            if(f) {
                fseek(f, 0, SEEK_SET);
                if(1 == fwrite(&head, sizeof(Header), 1, f)) {
                    cprintf<CPK::swap>("--> will use this one [%p]\n", f);
                    return new FileImpl(f, name);
                }
            }
        }
        cprintf<CPK::swap>("Failed to create any temp file\n");
        return nullptr;
    }

    ~FileImpl()
    {
        fclose(m_file);
        if(m_name != "") remove(m_name.c_str());
    }

    LinePtr head() override
    {
        return std::make_shared<FileLine>(m_file, offsetof(Header, head));
    }

    LinePtr cut() override
    {
        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);

        if(head.cut == 0) {
            cprintf<CPK::swap>("[%p] No cut buffer\n", m_file);
            return {};
        }

        cprintf<CPK::swap>("[%p] Cut buffer points to %I64d\n", m_file, head.cut);
        return std::make_shared<FileLine>(m_file, head.cut);
    }

    LinePtr undo() override
    {
        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);

        if(head.undo == 0) {
            cprintf<CPK::swap>("[%p] No undo buffer\n", m_file);
            return {};
        }

        cprintf<CPK::swap>("[%p] Undo buffer points to %I64d\n", m_file, head.undo);
        return std::make_shared<FileLine>(m_file, head.undo);
    }

    LinePtr line(Text const& s) override
    {
        fseek(m_file, 0, SEEK_END);
        fpos_t fp;
        memset(&fp, 0, sizeof(fp));
        fgetpos(m_file, &fp);

        cprintf<CPK::swap>("[%p] Adding line to %I64d\n", m_file, fp);
        int64_t zero = 0;
        fwrite(&zero, sizeof(int64_t), 1, m_file);
        uint16_t len = s.size();
        fwrite(&len, sizeof(uint16_t), 1, m_file);
        if(len == RefMagic) {
            fwrite(s.data(), sizeof(fpos_t), 1, m_file);
            cprintf<CPK::swap>("--> wrote indirect handle to %zd\n", (fpos_t)s.data());
        } else {
            fwrite(s.data(), 1, len, m_file);
            cprintf<CPK::swap>("--> wrote %u chars\n", len);
        }

        return std::make_shared<FileLine>(m_file, fp);
    }

    LinePtr cut(LinePtr const& p) override
    {
        auto pp = p.DownCast<FileLine>(); //std::dynamic_pointer_cast<FileLine>(p);

        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);

        cprintf<CPK::swap>("[%p] Cut buffer was %I64d\n", m_file, head.cut);

        head.cut = (pp) ? pp->m_pos : 0;
        fseek(m_file, 0, SEEK_SET);
        fwrite(&head, sizeof(Header), 1, m_file);
        cprintf<CPK::swap>("--> set cut buffer to %I64d\n", head.cut);

        return p;
    }

    LinePtr undo(LinePtr const& p)
    {
        auto pp = p.DownCast<FileLine>(); //std::dynamic_pointer_cast<FileLine>(p);

        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);
        cprintf<CPK::swap>("[%p] Undo buffer was %I64d\n", m_file, head.undo);

        head.undo = pp->m_pos;
        fseek(m_file, 0, SEEK_SET);
        fwrite(&head, sizeof(Header), 1, m_file);
        cprintf<CPK::swap>("--> set undo buffer to %I64d\n", head.undo);

        return p;
    }

#if 0
    void gc() override
    {
        cprintf<CPK::swap>("[%p] gcing swap file\n", m_file);
        auto cutBuffer = this->cut();
        auto undoBuffer = this->undo();
        auto l = this->head();

        std::unique_ptr<FileImpl> temp((FileImpl*)FileImpl::Create());
        cprintf<CPK::swap>("Relinking text\n");
        LinePtr prev = temp->head();
        for(l = l->next(); l; l = l->next()) {
            auto inserted = temp->line(l->text());
            prev->link(inserted);
            prev = inserted;
        }
        bool overlapTempCut = false;
        prev.reset();
        cprintf<CPK::swap>("Writing undo buffer\n");
        for(; undoBuffer; undoBuffer = undoBuffer->next()) {
            auto inserted = temp->line(undoBuffer->text());
            if(undoBuffer == cutBuffer) {
                temp->cut(inserted);
                overlapTempCut = true;
                cprintf<CPK::swap>("Detected overlap between undo and cut buffers\n");
            }
            if(prev) prev->link(inserted);
            else {
                prev = inserted;
                temp->undo(inserted);
            }
        }
        if(!overlapTempCut) {
            cprintf<CPK::swap>("Writing cut buffer\n");
            prev.reset();
            for(; cutBuffer; cutBuffer = cutBuffer->next()) {
                auto inserted = temp->line(cutBuffer->text());
                if(prev) prev->link(inserted);
                else {
                    prev = inserted;
                    temp->cut(inserted);
                }
            }
        }

        std::swap(m_file, temp->m_file);
        std::swap(m_name, temp->m_name);
    }
#endif
};

class MappedLine : public ILine
{
    friend class MappedImpl;
    MappedImpl& file;
    SIZE_T offset;
public:
#   pragma pack(push, 1)
    // I don't like counting, this let's me use offsetof()
    struct LineFormat
    {
        int64_t next;
        uint16_t sz;
        uint8_t text[16];
    };
#   pragma pack(pop)

    bool operator==(ILine const& other) const
    {
        auto pp = dynamic_cast<MappedLine const*>(&other);
        if(!pp) return false;
        cprintf<CPK::swap>("FileLine== %zd %zd\n", offset, pp->offset);
        if(&file != &pp->file) return false;
        if(offset != pp->offset) return false;
        return true;
    }

    MappedLine(MappedImpl& mimpl, SIZE_T zOffset)
        : file(mimpl)
        , offset(zOffset)
    {}

    size_t length() override;

    Text text() override;

    LinePtr next() override;

    void link(LinePtr const& p) override;

    LinePtr Copy() override
    {
        return std::make_shared<MappedLine>(file, offset);
    }

    Text ref() override;

    LinePtr deref() override;
};


class MappedImpl : public ISwapImpl
{
public:
#   pragma pack(push, 1)
    // I don't like counting, this let's me use offsetof()
    struct Header
    {
        int64_t head;
        uint16_t padding;
        int64_t cut;
        int64_t undo;
    };
#   pragma pack(pop)
    static constexpr SIZE_T LineBaseSize = sizeof(MappedLine::LineFormat) - sizeof(MappedLine::LineFormat::text);
    friend class MappedLine;
private:
    static constexpr SIZE_T zBaseSize = 1024;
    HANDLE m_file, m_map;
    SIZE_T m_baseSize, m_increment;
    SIZE_T m_size;
    uint8_t* m_view;
    std::string m_name;
    MappedImpl(HANDLE hFile, std::string name, HANDLE hMap, uint8_t* mView, SIZE_T sz)
        : m_file(hFile), m_name(name), m_map(hMap),
        m_increment(1), m_size(sizeof(Header)), m_baseSize(sz), m_view(mView)
    {
        cprintf<CPK::swap>("[%p] MappedImpl created\n", this);
        static_assert(zBaseSize > sizeof(Header), "Incorrect base size");
    }

    static std::string GetTEMPFileName()
    {
        static char tempDir[4096] = {'\0'};
        if(*tempDir == '\0') {
            DWORD length = GetTempPathA(sizeof(tempDir), tempDir);
            if(length <= 0 || length >= 4096) return "";
            tempDir[length] = '\0';
        }
        char lpTempFileName[MAX_PATH + 1] = {'\0'};
        LPCSTR lpPath = tempDir, lpPrefix = "jed";
        DWORD uUnique = 0;
        if(GetTempFileNameA(lpPath, lpPrefix, uUnique, lpTempFileName)) {
            cprintf<CPK::swap>("Computed temp file: %s\n", lpTempFileName);
            return lpTempFileName;
        }
        return "";
    }

    static std::string GetCWDFileName()
    {
        char lpTempFileName[MAX_PATH + 1] = {'\0'};
        LPCSTR lpPath = ".", lpPrefix = "jed";
        DWORD uUnique = 0;
        if(GetTempFileNameA(lpPath, lpPrefix, uUnique, lpTempFileName)) {
            cprintf<CPK::swap>("Computed temp file: %s\n", lpTempFileName);
            return lpTempFileName;
        }
        return "";
    }

    static constexpr SIZE_T GetNextIncrement(SIZE_T increment)
    {
        if(increment < 1) increment = 1;
        SIZE_T newSize = zBaseSize;
        while(--increment && newSize < (1 << 24)) { // 16MB
            newSize = newSize << 1;
        }
        if(increment > 0) return newSize * increment;
        return newSize;
    }

    void EnsureSize(SIZE_T growBy = 0)
    {
// I don't even know what null pointer gets dereferenced in the expression
//      m_size + growBy > m_baseSize
#pragma warning(push)
#pragma warning(disable:6011)
        if(m_size + growBy > m_baseSize) {
            cprintf<CPK::swap>("[%p] Swap file must grow, because %zd is smaller than %zd\n", this, m_baseSize, m_size + growBy);
            UnmapViewOfFile(m_view);
            CloseHandle(m_map);
            while(m_size + growBy > m_baseSize) {
                m_increment++;
                m_baseSize = GetNextIncrement(m_increment);
                cprintf<CPK::swap>("[%p] Swapfile increment computed to be %zd\n", this, m_baseSize);
            }
            m_map = CreateFileMapping(
                        m_file,
                        NULL,
                        PAGE_READWRITE,
                        (m_baseSize >> 32) & 0xFFFFFFFF,
                        m_baseSize & 0xFFFFFFFF,
                        NULL);
            if(!m_map) {
                // FIXME more graciousness?
                fprintf(stderr, "[%p] MappedImpl: I have forgotten. I have failed...\n", this);
                std::terminate();
            }
            m_view = nullptr;
        }
        if(!m_view) {
            cprintf<CPK::swap>("[%p] Creating view\n", this);
            m_view = (uint8_t*)MapViewOfFile(m_map, FILE_MAP_ALL_ACCESS, 0, 0, m_baseSize);
            Header* head = (Header*)m_view;
        }
#pragma warning(pop)
    }

public:

    static ISwapImpl* Create()
    {
        Header head;
        memset(&head, 0, sizeof(head));
        HANDLE hFile = (HANDLE)0, hMap = (HANDLE)0;
        uint8_t* mView = nullptr;
        SIZE_T sz = 0;

        std::string (*tempFunctions[])() = {
            GetTEMPFileName,
            GetCWDFileName,
        };
        for(auto&& tempFunction : tempFunctions) {
            auto tempName = tempFunction();
            auto* name = tempName.c_str();
            hFile = CreateFileA(
                    name,
                    GENERIC_READ|GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    /*FILE_ATTRIBUTE_TEMPORARY|*/FILE_FLAG_RANDOM_ACCESS, // but no DELETE_ON_CLOSE
                    NULL);
            if(hFile) {
                sz = GetNextIncrement(1);
                hMap = CreateFileMapping(
                        hFile,
                        NULL,
                        PAGE_READWRITE,
                        (sz >> 32) & 0xFFFFFFFF,
                        sz & 0xFFFFFFFF,
                        NULL);
                if(hMap) {
                    mView = (uint8_t*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sz);
                    if(!mView) {
                        CloseHandle(hMap);
                        CloseHandle(hFile);
                        hMap = (HANDLE)0;
                        hFile = (HANDLE)0;
                        mView = nullptr;
                        continue;
                    }

                    memcpy(mView, &head, sizeof(Header));
                    cprintf<CPK::swap>("--> will use this one %s [%p]\n", name, hFile);
                    return new MappedImpl(hFile, name, hMap, mView, sz);
                } else {
                    CloseHandle(hFile);
                }
            }
        }
        cprintf<CPK::swap>("Failed to create any temp file\n");
        return nullptr;
    }

    static ISwapImpl* Recover(std::string const& path)
    {
#if 0
        // get basesize from file size, and traverse each line to recompute size, and determine increment from basesize; no need to do shennanigans and store that info in the swapfile itself
        Header head;
        memset(&head, 0, sizeof(head));
        HANDLE hFile = (HANDLE)0, hMap = (HANDLE)0;
        uint8_t* mView = nullptr;
        SIZE_T sz = 0;

        std::string (*tempFunctions[])() = {
            GetTEMPFileName,
            GetCWDFileName,
        };
        auto* name = path.c_str();
        hFile = CreateFileA( // TODO CreateFileW
                name,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                /*FILE_ATTRIBUTE_TEMPORARY|*/FILE_FLAG_RANDOM_ACCESS, // but no DELETE_ON_CLOSE
                NULL);
        if(hFile) {
            hMap = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READWRITE,
                    (sz >> 32) & 0xFFFFFFFF,
                    sz & 0xFFFFFFFF,
                    NULL);
            sz = GetNextIncrement(1);
            if(hMap) {
                mView = (uint8_t*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sz);
                if(!mView) {
                    CloseHandle(hMap);
                    CloseHandle(hFile);
                    hMap = (HANDLE)0;
                    hFile = (HANDLE)0;
                    mView = nullptr;
                    return nullptr;
                }

                Header* thishead = (Header*)mView;
                auto oldSize = thishead->sz;

                auto rval = new MappedImpl(hFile, name, hMap, mView, thishead->basesz);
                thishead = nullptr; // clobbered by EnsureSize
                mView = nullptr; // clobbered by EnsureSize
                rval->EnsureSize(); // sets increment and recreates mapping and view to be what they should be
                rval->m_size = oldSize; // set the correct size
                cprintf<CPK::swap>("--> will use this one [%p]\n", hFile);
                return rval;
            } else {
                CloseHandle(hFile);
            }
        }
        cprintf<CPK::swap>("Failed to create any temp file\n");
#endif
        return nullptr;
    }

    ~MappedImpl()
    {
        if(m_view) UnmapViewOfFile(m_view);
        if(m_map) CloseHandle(m_map);
        if(m_file) CloseHandle(m_file);
        if(!m_name.empty()) DeleteFileA(m_name.c_str()); // TODO use DeleteFileW
        cprintf<CPK::swap>("Deleted %s maybe\n", m_name.c_str());
    }

    LinePtr head() override
    {
        return std::make_shared<MappedLine>(*this, offsetof(Header, head));
    }

    LinePtr cut() override
    {
        EnsureSize();
        Header* head = (Header*)(m_view);
        if(head->cut == 0) {
            cprintf<CPK::swap>("[%p] No cut buffer\n", m_file);
            return {};
        }

        cprintf<CPK::swap>("[%p] Cut buffer points to %I64d\n", m_file, head->cut);
        return std::make_shared<MappedLine>(*this, head->cut);
    }

    LinePtr undo() override
    {
        EnsureSize();
        Header* head = (Header*)(m_view);

        if(head->undo == 0) {
            cprintf<CPK::swap>("[%p] No undo buffer\n", m_file);
            return {};
        }

        cprintf<CPK::swap>("[%p] Undo buffer points to %I64d\n", this, head->undo);
        return std::make_shared<MappedLine>(*this, head->undo);
    }

    LinePtr line(Text const& s) override
    {
        EnsureSize(LineBaseSize
                + (s.size() == RefMagic ? (SIZE_T)sizeof(SIZE_T) : (SIZE_T)s.size()));
        Header* head = (Header*)(m_view);

        cprintf<CPK::swap>("[%p] Adding line to %zd\n", m_file, m_size);
        MappedLine::LineFormat* newLine = (MappedLine::LineFormat*)(m_view + m_size);
        newLine->next = 0;
        newLine->sz = s.size();
        if(newLine->sz == RefMagic) {
            memcpy(&newLine->text[0], s.data(), sizeof(SIZE_T));
            m_size += LineBaseSize + sizeof(SIZE_T);
        } else {
            memcpy(&newLine->text[0], s.data(), s.size());
            m_size += LineBaseSize + newLine->sz;
        }
        return std::make_shared<MappedLine>(*this, ((uint8_t*)newLine) - m_view);
    }

    LinePtr cut(LinePtr const& p) override
    {
        auto pp = p.DownCast<MappedLine>(); //std::dynamic_pointer_cast<FileLine>(p);
        EnsureSize();
        Header* head = (Header*)(m_view);

        cprintf<CPK::swap>("[%p] Cut buffer was %I64d\n", m_file, head->cut);

        head->cut = (pp) ? pp->offset : 0;
        cprintf<CPK::swap>("--> set cut buffer to %I64d\n", head->cut);

        return p;
    }

    LinePtr undo(LinePtr const& p)
    {
        auto pp = p.DownCast<MappedLine>(); //std::dynamic_pointer_cast<FileLine>(p);
        EnsureSize();
        Header* head = (Header*)(m_view);

        cprintf<CPK::swap>("[%p] Undo buffer was %I64d\n", m_file, head->undo);

        head->undo = (pp) ? pp->offset : 0;
        cprintf<CPK::swap>("--> set cut buffer to %I64d\n", head->undo);

        return p;
    }

#if 0
    void gc() override
    {
        cprintf<CPK::swap>("[%p] gcing swap file\n", m_file);
        auto cutBuffer = this->cut();
        auto undoBuffer = this->undo();
        auto l = this->head();

        std::unique_ptr<MappedImpl> temp((MappedImpl*)MappedImpl::Create());
        cprintf<CPK::swap>("Relinking text\n");
        LinePtr prev = temp->head();
        for(l = l->next(); l; l = l->next()) {
            auto inserted = temp->line(l->text());
            prev->link(inserted);
            prev = inserted;
        }
        bool overlapTempCut = false;
        prev.reset();
        cprintf<CPK::swap>("Writing undo buffer\n");
        for(; undoBuffer; undoBuffer = undoBuffer->next()) {
            auto inserted = temp->line(undoBuffer->text());
            if(undoBuffer == cutBuffer) {
                temp->cut(inserted);
                overlapTempCut = true;
                cprintf<CPK::swap>("Detected overlap between undo and cut buffers\n");
            }
            if(prev) prev->link(inserted);
            else {
                prev = inserted;
                temp->undo(inserted);
            }
        }
        if(!overlapTempCut) {
            cprintf<CPK::swap>("Writing cut buffer\n");
            prev.reset();
            for(; cutBuffer; cutBuffer = cutBuffer->next()) {
                auto inserted = temp->line(cutBuffer->text());
                if(prev) prev->link(inserted);
                else {
                    prev = inserted;
                    temp->cut(inserted);
                }
            }
        }

        std::swap(m_file, temp->m_file);
        std::swap(m_map, temp->m_map);
        std::swap(m_baseSize, temp->m_baseSize);
        std::swap(m_increment, temp->m_increment);
        std::swap(m_view, temp->m_view);
        std::swap(m_size, temp->m_size);
        std::swap(m_name, temp->m_name);
    }
#endif
};

inline size_t MappedLine::length()
{
    LineFormat* me = (LineFormat*)(file.m_view + offset);
    return me->sz;
}

inline Text MappedLine::text()
{
    LineFormat* me = (LineFormat*)(file.m_view + offset);
    if(me->sz == RefMagic) throw std::runtime_error("Invalid indirection");
    return Text(me->sz, &me->text[0]);
}

inline LinePtr MappedLine::next()
{
    LineFormat* me = (LineFormat*)(file.m_view + offset);
    if(me->next) return std::make_shared<MappedLine>(file, me->next);
    return {};
}

inline void MappedLine::link(LinePtr const& p)
{
    auto pp = p.DownCast<MappedLine>();
    LineFormat* me = (LineFormat*)(file.m_view + offset);
    if(!pp) {
        me->next = 0;
        cprintf<CPK::swap>("[%p] link: %zd -> %zd\n", &file, offset, 0ull);
    } else {
        me->next = pp->offset;
        cprintf<CPK::swap>("[%p] link: %zd -> %zd\n", &file, offset, pp->offset);
    }
}

inline Text MappedLine::ref()
{
    LineFormat* me = (LineFormat*)(file.m_view + offset);
    return Text(RefMagic, (uint8_t*)&me->text[0]);
}

inline LinePtr MappedLine::deref()
{
    LineFormat* me = (LineFormat*)(file.m_view + offset);
    return std::make_shared<MappedLine>(file, *(decltype(offset)*)&me->text[0]);
}

class MemoryImpl : public FileImpl
{
    // TODO
};

struct NullImpl: public FileImpl
{
};

Swapfile::Swapfile(int type) : m_pImpl(nullptr)
{
    ISwapImpl* (*factories[])() = {
        &MappedImpl::Create,
        &FileImpl::Create,
        &MemoryImpl::Create,
        &NullImpl::Create,
        nullptr
    };
    for(auto f = factories; f; ++f) {
        m_pImpl = (*f)();
        if(m_pImpl) return;
    }
}

void Swapfile::type(int t)
{
    t = 3;
    ISwapImpl* (*factories[])() = {
        &NullImpl::Create,
        &MemoryImpl::Create,
        &FileImpl::Create,
        &MappedImpl::Create,
        nullptr
    };
    if(t < 0 || t > sizeof(factories) / sizeof(factories[0])) std::terminate();
    delete m_pImpl;
    m_pImpl = (factories[t])();
}

Swapfile::Swapfile(ISwapImpl* impl) : m_pImpl(impl) {}

Swapfile::~Swapfile()
{
    delete m_pImpl;
}

LinePtr Swapfile::head() { return m_pImpl->head(); }
LinePtr Swapfile::cut() { return m_pImpl->cut(); }
LinePtr Swapfile::undo() { return m_pImpl->undo(); }
LinePtr Swapfile::line(Text const& s) { return m_pImpl->line(s); }
LinePtr Swapfile::undo(LinePtr const& l) { return m_pImpl->undo(l); }
LinePtr Swapfile::cut(LinePtr const& l) { return m_pImpl->cut(l); }
//void Swapfile::gc() { return m_pImpl->gc(); }

