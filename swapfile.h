#ifndef JAKED_SWAPFILE_H
#define JAKED_SWAPFILE_H

#include <string>
#include <list>

struct ISwapImpl
{
    virtual void a(std::list<std::string> const&) = 0;
    virtual void yank(std::list<std::string> const&) = 0;
    virtual std::list<std::string> paste() = 0;
    virtual void w() = 0;
    virtual std::list<std::string> undo() = 0;
    virtual void setUndo(std::string const&, std::list<std::string> const&) = 0;
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

    void a(std::list<std::string> const&);
    void yank(std::list<std::string> const&);
    std::list<std::string> paste();
    void w();
    std::list<std::string> undo();
    void setUndo(std::string const&, std::list<std::string> const&);
};

#endif
