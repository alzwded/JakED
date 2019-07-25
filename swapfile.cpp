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

#undef DEBUG_SWAPFILE

class FileImpl : public ISwapImpl
{
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
            return lpTempFileName;
        }
        return "";
    }

public:
#   pragma pack(push, 1)
    struct Header
    {
        int32_t cutOffset;
        int32_t cutLen;
        int32_t undoOffset;
        int32_t undoLen;
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
                    return new FileImpl(f, name);
                }
            }
        }
        return nullptr;
    }

    ~FileImpl()
    {
        fclose(m_file);
        if(m_name != "") remove(m_name.c_str());
    }

    void a(std::list<std::string> const& lines) override
    {
        for(auto&& line : lines)
        {
            fwrite(line.c_str(), 1, line.size(), m_file);
        }
    }

    void yank(std::list<std::string> const& lines) override
    {
        // I've heard online that one must fseek(0, CUR) for ftell
        // to work when switching between R&W in update mode; I even
        // saw it written in some man 3 page somewhere. I could not
        // find it in the last publicly published C standard draft,
        // so IDK, based on pure observation, the first statement is
        // correct. Maybe I haven't read the standard fully
        fseek(m_file, 0, SEEK_CUR);
        auto pos = ftell(m_file);

        uint32_t nBytes = 0;
        for(auto&& line : lines)
        {
            nBytes += line.size() + 1;
            fwrite(line.c_str(), 1, line.size(), m_file);
            fwrite("\n", 1, 1, m_file);
        }
        fseek(m_file, 0, SEEK_CUR);
        auto endOfFile = ftell(m_file);
#ifdef DEBUG_SWAPFILE
        printf("pos = %d eof = %d\n", pos, endOfFile);
#endif

        int32_t asInt = pos & 0xFFFFFFFF;

        Header head;
        fseek(m_file, 0, SEEK_SET);
        fread(&head, sizeof(Header), 1, m_file);

        fseek(m_file, 0, SEEK_SET);
        head.cutOffset = pos;
        head.cutLen = nBytes;
        fwrite(&head, sizeof(Header), 1, m_file);
#ifdef DEBUG_SWAPFILE
        printf("writing cut SF header %d %d %d %d\n", head.cutOffset, head.cutLen, head.undoOffset, head.undoLen);
#endif

        fseek(m_file, endOfFile, SEEK_SET);
#ifdef DEBUG_SWAPFILE
        printf("bbq %d\n", ftell(m_file));
#endif
    }

    void setUndo(
            std::string const& command,
            std::list<std::string> const& lines)
        override
    {
        fseek(m_file, 0, SEEK_CUR);
        auto pos = ftell(m_file);

        uint32_t nBytes = command.size();
        fwrite(command.c_str(), 1, command.size(), m_file);
        fwrite("\n", 1, 1, m_file);
        for(auto&& line : lines)
        {
            nBytes += line.size() + 1;
            fwrite(line.c_str(), 1, line.size(), m_file);
            fwrite("\n", 1, 1, m_file);
        }
        fseek(m_file, 0, SEEK_CUR);
        auto endOfFile = ftell(m_file);
#ifdef DEBUG_SWAPFILE
        printf("pos = %d eof = %d\n", pos, endOfFile);
#endif

        int32_t asInt = pos & 0xFFFFFFFF;

        Header head;
        fseek(m_file, 0, SEEK_SET);
        fread(&head, sizeof(Header), 1, m_file);

        fseek(m_file, 0, SEEK_SET);
        head.undoOffset = pos;
        head.undoLen = nBytes;
        fwrite(&head, sizeof(Header), 1, m_file);
#ifdef DEBUG_SWAPFILE
        printf("writing undo SF header %d %d %d %d\n", head.cutOffset, head.cutLen, head.undoOffset, head.undoLen);
#endif

        fseek(m_file, endOfFile, SEEK_SET);
#ifdef DEBUG_SWAPFILE
        printf("bbq %d\n", ftell(m_file));
#endif
    }

    std::list<std::string> readLines(int32_t offset, uint32_t length)
    {
        std::list<std::string> rval;

        fseek(m_file, offset, SEEK_SET);
        char buf[1024];
        bool waiting = false;
        while(length > 0) {
            int len = std::min(length, (uint32_t)(sizeof(buf) - 1));
            fread(buf, 1, len, m_file);
            length -= len;
            buf[sizeof(buf) - 1] = '\0';
            char* p = buf, *q;
            while((p < buf + len)
                    && (q = strchr(p, '\n')))
            {
                *q = '\0';
                if(!waiting) rval.emplace_back(p);
                else {
                    rval.back() = rval.back() + p;
                    waiting = false;
                }
                p = q + 1;
            }
            if(p < buf + len) {
                rval.emplace_back(p);
                waiting = true;
            }
        }
        fseek(m_file, 0, SEEK_END);
        return rval;
    }

    std::list<std::string> paste() override
    {
        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);
        if(head.cutLen == 0) return {};
#ifdef DEBUG_SWAPFILE
        printf("SF header %d %d %d %d\n", head.cutOffset, head.cutLen, head.undoOffset, head.undoLen);
#endif

        return readLines(head.cutOffset, head.cutLen);
    }

    std::list<std::string> undo() override
    {
        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);
        if(head.undoLen == 0) return {};

        return readLines(head.undoOffset, head.undoLen);
    }

    void w() override
    {
        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);

        auto cutBuffer = paste();
        auto undoBuffer = undo();
        fseek(m_file, sizeof(Header), SEEK_SET);
        yank(cutBuffer);
        auto command = undoBuffer.front();
        undoBuffer.pop_front();
        setUndo(command, undoBuffer);

        HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(m_file));
        SetEndOfFile(hFile);
    }
};

class MemoryImpl : public ISwapImpl
{
    std::list<std::string> m_cutBuffer;
    std::list<std::string> m_undoBuffer;
    MemoryImpl() : m_cutBuffer() {}
public:
    static ISwapImpl* Create() { return new MemoryImpl(); }

    void a(std::list<std::string> const&) override {}
    void yank(std::list<std::string> const& lines) override
    {
        m_cutBuffer = lines;
    }
    std::list<std::string> paste() override
    {
        return m_cutBuffer;
    }
    void w() override {}
    std::list<std::string> undo() override
    {
        return m_undoBuffer;
    }
    void setUndo(
            std::string const& command,
            std::list<std::string> const& lines)
        override
    {
        m_undoBuffer = lines;
    }

};

struct NullImpl: public ISwapImpl
{
    static ISwapImpl* Create()
    {
        return new NullImpl();
    }

    void a(std::list<std::string> const& lines) override {
        fprintf(stderr, "Failed to create swap file.\n");
    }
    void yank(std::list<std::string> const& lines) override {
        fprintf(stderr, "Failed to create swap file.\n");
    }
    std::list<std::string> paste() override { return {}; }
    void w() override {}
    std::list<std::string> undo() override { return {}; }
    void setUndo(std::string const&, std::list<std::string> const&) override {
        fprintf(stderr, "Failed to create swap file.\n");
    }
};

Swapfile::Swapfile() : m_pImpl(nullptr)
{
    ISwapImpl* (*factories[])() = {
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
    ISwapImpl* (*factories[])() = {
        &NullImpl::Create,
        &MemoryImpl::Create,
        &FileImpl::Create,
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

void Swapfile::a(std::list<std::string> const& lines)
{
    return m_pImpl->a(lines);
}

void Swapfile::yank(std::list<std::string> const& lines)
{
    return m_pImpl->yank(lines);
}

std::list<std::string> Swapfile::paste()
{
    return m_pImpl->paste();
}

void Swapfile::w()
{
    return m_pImpl->w();
}

std::list<std::string> Swapfile::undo()
{
    return m_pImpl->undo(); 
}

void Swapfile::setUndo(
        std::string const& command,
        std::list<std::string> const& lines)
{
    return m_pImpl->setUndo(command, lines);
}
