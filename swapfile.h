#ifndef JAKED_SWAPFILE_H
#define JAKED_SWAPFILE_H

#include <string>
#include <memory>


/*
   file structure:
   u64 pCut
   u64 pUndo
   u64 pHead
   u16 padding
   Line{*}
   --

   Line: 
       u64 next
       u16 sz
       char[sz] text


   Example state:
   42                           0
   122                          8
   26                           16
   0x0000                       24
   74 6 'Line 1'                26
   0 6 'Line 2'                 42
   0 ? Line 3                   58
   98 14 'Replace line 1'       74
   58 14 'Replace line 2'       98
   42 4 '2,3c'                  122
                                EOF

   Summary: Started with a 3 line file
            Executed 2c\nReplace line 1\nReplace line 2\n.
*/

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
    virtual size_t length() = 0;
    virtual std::string text() = 0;
    virtual LinePtr next() = 0;
    inline void link() { return link(LinePtr()); }
    inline void link(nullptr_t) { return link(LinePtr()); }
    virtual void link(LinePtr const& p) = 0;
    virtual bool operator==(ILine const& other) const = 0;
    virtual LinePtr Copy() = 0;
};
inline bool LinePtr::operator==(LinePtr const& right) const
{
    auto left = *this;
    printf("operator==, existance %p %p\n", (*this).operator->(), right.operator->());
    if(left && right) return (*left) == (*right);
    if(!left && !right) return true;
    return false;
}

struct ISwapImpl
{
    virtual LinePtr head() = 0;
    virtual LinePtr cut() = 0;
    virtual LinePtr undo() = 0;
    virtual LinePtr line(std::string const&) = 0;
    virtual LinePtr cut(LinePtr const&) = 0;
    virtual LinePtr undo(LinePtr const&) = 0;
    /**
      * Rebuild the swap file. All LinePtr's will become invalid.
      * 
      * This compacts the swap file. Should be invoked after every write.
      */
    virtual void Rebuild() = 0;
};

class Swapfile
{
    ISwapImpl* m_pImpl;
public:
    enum {
        DISABLE_SWAPFILE,
        IN_MEMORY_SWAPFILE,
        FILE_BACKED_SWAPFILE /** the default */
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
      */
    void type(int t);

    LinePtr head();
    LinePtr cut();
    LinePtr undo();
    LinePtr line(std::string const&);
    LinePtr cut(LinePtr const&);
    LinePtr undo(LinePtr const&);
    /**
      * Rebuild the swap file. All LinePtr's will become invalid.
      * 
      * This compacts the swap file. Should be invoked after every write.
      */
    void Rebuild();
};

#endif
