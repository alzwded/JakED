#ifndef COMMON_H
#define COMMON_H

#include <atomic>

#ifndef ISATTY
# define ISATTY(X) _isatty(X)
#endif

extern volatile std::atomic<bool> ctrlc;
extern volatile std::atomic<bool> ctrlint;
inline bool CtrlC()
{
    return (ctrlc = ctrlc.load())
        || (ctrlint = ctrlint.load());
}

#endif
