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

class FileImpl;
class FileLine : public ILine
{
    friend class FileImpl;
    FILE* const m_file;
    static_assert(std::is_convertible<fpos_t, uint64_t>::value, "fpos_t is not convertible to uint64_t");
    static_assert(std::is_convertible<uint64_t, fpos_t>::value, "fpos_t is not convertible to uint64_t");
    static_assert(sizeof(fpos_t) <= sizeof(uint64_t), "fpos_t is not convertible to uint64_t");
    uint64_t m_pos;
public:
#   pragma pack(push, 1)
    struct LineFormat
    {
        uint64_t next;
        uint16_t sz;
        char* text;
    };
#   pragma pack(pop)

    bool operator==(ILine const& other) const
    {
        auto pp = dynamic_cast<FileLine const*>(&other);
        if(!pp) return false;
        //printf("%zd %zd\n", m_pos, pp->m_pos);
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
        return len;
    }

    std::string text() override
    {
        fpos_t offset = m_pos + offsetof(LineFormat, sz);
        fsetpos(m_file, &offset);
        uint16_t len = 0;
        fread(&len, 2, 1, m_file);
        std::string rval(len, '\0');
        offset = m_pos + offsetof(LineFormat, text);
        fsetpos(m_file, &offset);
        fread(rval.data(), 1, len, m_file);
        return rval;
    }

    LinePtr next() override
    {
        fpos_t offset = m_pos + offsetof(LineFormat, next);
        fsetpos(m_file, &offset);
        uint64_t nextPos = 0;
        fread(&nextPos, sizeof(uint64_t), 1, m_file);
        if(nextPos == 0) return {};
        return std::make_shared<FileLine>(m_file, nextPos);
    }

    void link(LinePtr const& p) override
    {
        auto pp = p.DownCast<FileLine>(); //std::dynamic_pointer_cast<FileLine>(p);
        fpos_t offset = m_pos + offsetof(LineFormat, next);
        fsetpos(m_file, &offset);
        uint64_t nextPos = (pp) ? pp->m_pos : 0;
        fwrite(&nextPos, sizeof(uint64_t), 1, m_file);
    }

    LinePtr Copy() override
    {
        return std::make_shared<FileLine>(m_file, m_pos);
    }
};

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
        uint64_t head;
        uint64_t cut;
        uint64_t undo;
        uint16_t padding;
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

        return std::make_shared<FileLine>(m_file, head.cut);
    }

    LinePtr undo() override
    {
        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);

        return std::make_shared<FileLine>(m_file, head.undo);
    }

    LinePtr line(std::string const& s) override
    {
        fseek(m_file, 0, SEEK_END);
        fpos_t fp;
        memset(&fp, 0, sizeof(fp));
        fgetpos(m_file, &fp);

        uint64_t zero = 0;
        fwrite(&zero, sizeof(uint64_t), 1, m_file);
        uint16_t len = (uint16_t)std::min(s.size(), (size_t)0xFFFF);
        fwrite(&len, sizeof(uint16_t), 1, m_file);
        fwrite(s.data(), 1, len, m_file);

        return std::make_shared<FileLine>(m_file, fp);
    }

    LinePtr cut(LinePtr const& p) override
    {
        auto pp = p.DownCast<FileLine>(); //std::dynamic_pointer_cast<FileLine>(p);

        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);

        head.cut = (pp) ? pp->m_pos : 0;
        fseek(m_file, 0, SEEK_SET);
        fwrite(&head, sizeof(Header), 1, m_file);

        return p;
    }

    LinePtr undo(LinePtr const& p)
    {
        auto pp = p.DownCast<FileLine>(); //std::dynamic_pointer_cast<FileLine>(p);

        fseek(m_file, 0, SEEK_SET);
        Header head;
        memset(&head, 0, sizeof(Header));
        fread(&head, sizeof(Header), 1, m_file);

        head.undo = pp->m_pos;
        fseek(m_file, 0, SEEK_SET);
        fwrite(&head, sizeof(Header), 1, m_file);

        return p;
    }

    void Rebuild() override
    {
        auto cutBuffer = this->cut();
        auto undoBuffer = this->undo();
        auto l = this->head();

        std::unique_ptr<FileImpl> temp((FileImpl*)FileImpl::Create());
        LinePtr prev = temp->head();
        for(; l; l = l->next()) {
            auto inserted = temp->line(l->text());
            prev->link(inserted);
        }
        bool overlapTempCut = false;
        prev.reset();
        for(; undoBuffer; undoBuffer = undoBuffer->next()) {
            auto inserted = temp->line(undoBuffer->text());
            if(undoBuffer == cutBuffer) {
                temp->cut(inserted);
                overlapTempCut = true;
            }
            if(prev) prev->link(inserted);
            else {
                prev = inserted;
                temp->undo(inserted);
            }
        }
        if(!overlapTempCut) {
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
};

class MemoryImpl : public FileImpl
{
};

struct NullImpl: public FileImpl
{
};

Swapfile::Swapfile(int type) : m_pImpl(nullptr)
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
    t = 2;
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

LinePtr Swapfile::head() { return m_pImpl->head(); }
LinePtr Swapfile::cut() { return m_pImpl->cut(); }
LinePtr Swapfile::undo() { return m_pImpl->undo(); }
LinePtr Swapfile::line(std::string const& s) { return m_pImpl->line(s); }
LinePtr Swapfile::undo(LinePtr const& l) { return m_pImpl->undo(l); }
LinePtr Swapfile::cut(LinePtr const& l) { return m_pImpl->cut(l); }
void Swapfile::Rebuild() { return m_pImpl->Rebuild(); }

