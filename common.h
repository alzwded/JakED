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

struct application_exit : std::exception
{
    bool m_error;
    application_exit(bool error) : std::exception("This should not be reported as an error"), m_error(error) {}
    bool ExitedWithError() const { return m_error; }
};

extern "C" const char VERSION[];

#endif
