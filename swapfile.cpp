#include "swapfile.h"

#include <vector>
#include <list>
#include <cstdio>
#include <string>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>

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
    static ISwapImpl* Create()
    {
        static char padding[] = 
            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // pos of cut buffer
            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" // len of cut buffer
            ;

        // try tmpfile
        FILE *f = tmpfile();
        if(f) {
            fseek(f, 0, SEEK_SET);
            if(32 == fwrite(padding, 1, sizeof(int32_t) + sizeof(uint32_t), f)) {
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
                if(32 == fwrite(padding, 1, sizeof(int32_t) + sizeof(uint32_t), f)) {
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
            fprintf(m_file, "%s\n", line.c_str());
        }
    }

    void yank(std::list<std::string> const& lines) override
    {
        auto pos = ftell(m_file);
        uint32_t nBytes = 0;
        for(auto&& line : lines)
        {
            nBytes += line.size() + 1;
            fprintf(m_file, "%s\n", line.c_str());
        }
        fseek(m_file, 0, SEEK_SET);
        int32_t asInt = pos & 0xFFFFFFFF;
        fwrite(&asInt, sizeof(int32_t), 1, m_file);
        fwrite(&nBytes, sizeof(uint32_t), 1, m_file);
        fseek(m_file, 0, SEEK_END);
    }

    std::list<std::string> paste() override
    {
        fseek(m_file, 0, SEEK_SET);
        int32_t pos;
        uint32_t len;
        fread(&pos, sizeof(int32_t), 1, m_file);
        fread(&len, sizeof(uint32_t), 1, m_file);
        std::vector<char> buffer('\0', pos + 1);
        fseek(m_file, pos, SEEK_SET);
        fread(buffer.data(), sizeof(char), len, m_file);
        fseek(m_file, 0, SEEK_END);
        std::stringstream ss;
        ss << buffer.data();
        std::list<std::string> rval;
        while(!ss.eof()) {
            std::string line;
            getline(ss, line, '\n');
            rval.push_back(std::move(line));
        }
        return rval;
    }

    void w() override
    {
        fseek(m_file, 0, SEEK_SET);
        int32_t pos;
        uint32_t len;
        fread(&pos, sizeof(int32_t), 1, m_file);
        fread(&len, sizeof(uint32_t), 1, m_file);
        std::vector<char> buffer('\0', pos + 1);
        fseek(m_file, pos, SEEK_SET);
        fread(buffer.data(), sizeof(char), len, m_file);
        fseek(m_file, sizeof(int32_t) + sizeof(uint32_t), SEEK_SET);
        fwrite(buffer.data(), sizeof(char), len, m_file);

        HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(m_file));
        SetEndOfFile(hFile);
    }
};

class MemoryImpl : public ISwapImpl
{
    std::list<std::string> m_cutBuffer;
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

};

struct NullImpl: public ISwapImpl
{
    static ISwapImpl* Create()
    {
        fprintf(stderr, "Failed to create swap file.\n");
        return new NullImpl();
    }

    void a(std::list<std::string> const& lines) override {}
    void yank(std::list<std::string> const& lines) override {}
    std::list<std::string> paste() override { return {}; }
    void w() override {}
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
