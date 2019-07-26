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

struct ILine
{
    virtual size_t length() = 0;
    virtual std::string text() = 0;
    virtual std::shared_ptr<ILine> next() = 0;
    inline void link() { return link(std::shared_ptr<ILine>()); }
    inline void link(nullptr_t) { return link(std::shared_ptr<ILine>()); }
    virtual void link(std::shared_ptr<ILine> const& p) = 0;
    virtual bool operator==(ILine const& other) const = 0;
    virtual std::shared_ptr<ILine> Copy() = 0;
};
typedef std::shared_ptr<ILine> LinePtr;

inline bool operator==(LinePtr const& left, LinePtr const& right)
{
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
    Swapfile();
    ~Swapfile();
    Swapfile(ISwapImpl*);
    Swapfile(Swapfile const&) = delete;
    Swapfile& operator=(Swapfile const&) = delete;
    enum {
        DISABLE_SWAPFILE,
        IN_MEMORY_SWAPFILE,
        FILE_BACKED_SWAPFILE /** the default */
    };
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
