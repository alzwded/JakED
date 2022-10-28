/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef JAKED_SWAPFILE_H
#define JAKED_SWAPFILE_H

#include <string>
#include <memory>
#include <functional>

#include "cprintf.h"


/*
   file structure:
   u64 pHead
   u16 padding
   u64 pCut
   u64 pUndo
   Line{*}
   --

   Line: 
       u64 next
       u16 sz
       char[sz] text

   Example state:
   26                           0               head -> Line 1
   0x0000                       8               always 0
   0                            10              NUL
   0                            18              NUL
   42 6 'Line 1'                26              Line 1 -> Line 2
   58 6 'Line 2'                42              Line 2 -> Line 3
   0  6 'Line 3'                58              Line 3 -> EOF
                                EOF

   Execute 2c\nReplace line 1\nReplace line 2\n.

   Example state:
   26                           0               head -> Line 1
   0x0000                       8               always 0
   42                           10              cut -> Line 2
   122                          18              undo -> 2,3c
   74 6 'Line 1'                26              Line 1 -> Replaced line 1
   0  6 'Line 2'                42              Line 2 -> EOF
   0  6 'Line 3'                58              Line 3 -> EOF
   98 14 'Replace line 1'       74              Rep...ine 1 -> R... line 2
   58 14 'Replace line 2'       98              Rep...ine 2 -> Line 3
   140 8 0x00000000             122             prev undo -> 0, undo command -> '2,3c'
   42 4  '2,3c'                 140             2,3c -> Line 2
                                EOF

   Summary: Started with a 3 line file
            Executed 2c\nReplace line 1\nReplace line 2\n.\n

   Execute 2d

   Example state:
   26                           0               head -> Line 1
   0x0000                       8               always 0
   42                           10              cut -> Line 2
   122                          18              undo -> 2,3c
   74 6 'Line 1'                26              Line 1 -> Replaced line 1
   0  6 'Line 2'                42              Line 2 -> EOF
   0  6 'Line 3'                58              Line 3 -> EOF
   58 14 'Replace line 1'       74              Rep...ine 1 -> Line 3
   58 14 'Replace line 2'       98              Rep...ine 2 -> Line 3
   140 -1 0x00000000             122             prev undo -> 0, undo command -> '2,3c'
   42 4  '2,3c'                 140             2,3c -> Line 2
   172 -1 0x0000007A             154             prev undo -> 140, undo command -> '2i'
   98 2 '2i'                    172             2i -> Replace line 2
                                EOF

   Summary: Started with a 3 line file
            Executed 2c\nReplace line 1\nReplace line 2\n.\n2d\n

*/

class Swapfile;

static constexpr uint16_t RefMagic = (uint16_t)-1;

struct Text {
    uint16_t size() const { return _size; }
    uint8_t *const data() const { return _data; }
    Text(uint16_t sz, uint8_t *d, std::function<void(void*)> freeFunc = std::function<void(void*)>(&nop) /* leave this nop; it's there for ISwapFile implementations */)
        : _size(sz)
        , _data(d)
        , _free(freeFunc)
    {}
    Text(Text&& other)
        : _size(0)
        , _data(nullptr)
        , _free(std::function<void(void*)>(&nop))
    {
        std::swap(_size, other._size);
        std::swap(_data, other._data);
        std::swap(_free, other._free);
    }
    Text& operator=(Text&& other)
    {
        _free(_data);
        _size = 0;
        _data = nullptr;
        _free = std::function<void(void*)>(&nop);
        std::swap(_size, other._size);
        std::swap(_data, other._data);
        std::swap(_free, other._free);
        return *this;
    }
    static void nop(void*) {}
    ~Text() { _free(_data); }
    Text(Text const&) = delete;
    Text& operator=(Text const&) = delete;
    operator std::string() {
        if(_size == RefMagic) throw std::runtime_error("attempting to dereference an indirect handle as an std::string");
        return std::string((const char*)_data, (size_t)_size);
    }
private:
    uint16_t _size;
    uint8_t *_data;
    std::function<void(void*)> _free;
};

struct ILine;
struct LinePtr
{
    std::shared_ptr<ILine> p;
    LinePtr(std::shared_ptr<ILine> const& pp = {}) : p(pp) {}
    bool operator==(LinePtr const& right) const;
    inline bool operator!=(LinePtr const& right) const { return !operator==(right); }
    explicit operator bool() const { return p.operator bool(); }
    ILine* operator->() const { return p.operator->(); }
    ILine& operator*() const { return p.operator*(); }
    template<typename T>
    T* DownCast() const { return dynamic_cast<T*>(p.get()); }
    void reset(ILine* pp = nullptr) { p.reset(pp); }
};
struct ILine
{
    // returns the length of the string, sz
    virtual size_t length() = 0;
    // returns the text of the string, char[sz]
    virtual Text text() = 0;
    // returns the linked line
    virtual LinePtr next() = 0;
    // unlinks the next line
    inline void link() { return link(LinePtr()); }
    // unlinks the next line
    inline void link(nullptr_t) { return link(LinePtr()); }
    // links a line to this one
    virtual void link(LinePtr const& p) = 0;
    // compare line refs
    virtual bool operator==(ILine const& other) const = 0;
    // duplicates the handle
    virtual LinePtr Copy() = 0;
    // returns a magic string used to create an indirect handle to this line
    virtual Text ref() = 0;
    // check if this LinePtr is an indirect handle
    bool IsIndirectHandle() {
        return length() == RefMagic;
    }
    // dereference an indirect handle
    virtual LinePtr deref() = 0;
};
inline bool LinePtr::operator==(LinePtr const& right) const
{
    auto left = *this;
    cprintf<CPK::swap>("operator==, existance %p %p\n", (*this).operator->(), right.operator->());
    if(left && right) return (*left) == (*right);
    if(!left && !right) return true;
    return false;
}

struct ISwapImpl
{
    virtual ~ISwapImpl();
    virtual LinePtr head() = 0;
    virtual LinePtr cut() = 0;
    virtual LinePtr undo() = 0;
    virtual LinePtr line(Text const&) = 0;
    virtual LinePtr cut(LinePtr const&) = 0;
    virtual LinePtr undo(LinePtr const&) = 0;
#if 0
    /**
      * gc the swap file. All LinePtr's will become invalid.
      * 
      * This compacts the swap file. Should be invoked after every write.
      */
    virtual void gc() = 0;
#endif
private:
    friend class Swapfile;
    virtual int64_t undoi(bool) = 0;
    virtual void undoi(int64_t) = 0;
};

inline ISwapImpl::~ISwapImpl() = default;

class Swapfile
{
    ISwapImpl* m_pImpl;
public:
    enum {
        DISABLE_SWAPFILE,
        IN_MEMORY_SWAPFILE,
        FILE_BACKED_SWAPFILE, /** the default */
        MAPPED_FILE_BACKED_SWAPFILE
    };
    Swapfile(int type = IN_MEMORY_SWAPFILE);
    Swapfile() = delete;
    Swapfile(Swapfile&& other)
        : m_pImpl(nullptr)
    {
        std::swap(m_pImpl, other.m_pImpl);
    }
    ~Swapfile();
    Swapfile(ISwapImpl*);
    Swapfile(Swapfile const&) = delete;
    Swapfile& operator=(Swapfile const&) = delete;
    /**
      * t: in, int
      *    0 = disable swap file
      *    1 = in-memory
      *    2 = file-backed
      *    3 = mapped file-backed
      */
    void type(int t);

    LinePtr head();
    LinePtr cut();
    LinePtr undo();
    LinePtr line(Text const&);
    LinePtr line(std::string const& s)
    {
        return line(Text((uint16_t)s.size(), (uint8_t*)s.c_str()));
    }
    LinePtr cut(LinePtr const&);
    LinePtr undo(LinePtr const&);
    /**
      * gc the swap file. All LinePtr's will become invalid.
      * 
      * This compacts the swap file. Should be invoked after every write.
      */
    //void gc();

    // call this before begining to execute reverse undo commands
    int64_t popUndo();
    // call this if you want to preserve the current undo mark
    int64_t saveUndo();
    // call this after finishing to execute reverse undo commands
    void restoreUndo(int64_t);
};

#endif
