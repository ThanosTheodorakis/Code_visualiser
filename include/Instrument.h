// Instrument.h - Macros that instrument if, for and while once a program includes this header.

#include <utility>

#include "Markers.h"

#ifndef FV_INSTRUMENT_TYPES
#define FV_INSTRUMENT_TYPES

namespace fv {
// Set when the next if is the check of a loop pass, not a real branch.
inline bool& iterationFlag()
{
    static thread_local bool flag = false;
    return flag;
}

// Sets iterationFlag when tested; always false.
struct IterationMark
{
    explicit operator bool() const
    {
        iterationFlag() = true;
        return false;
    }
};

// A condition value with its text and whether it is a loop pass.
struct Condition
{
    bool value;
    bool iteration;
    const char* text;
};

// Wraps a condition and takes over the iteration flag.
inline Condition asCondition(bool value, const char* text)
{
    const bool iteration = iterationFlag();
    iterationFlag() = false;
    return Condition{value, iteration, text};
}

// Reports an if (or a loop pass) for as long as it lives.
class BranchGuard
{
public:
    explicit BranchGuard(Condition condition) : condition_(condition.value), iteration_(condition.iteration)
    {
        if (iteration_) {
            Trackers::trackLoopIteration();
        } else {
            Trackers::trackBranchEnter(condition_, cleanSource(condition.text));
        }
    }

    ~BranchGuard()
    {
        if (!iteration_) {
            Trackers::trackBranchExit(condition_);
        }
    }

    explicit operator bool() const { return condition_; }

    BranchGuard(const BranchGuard&) = delete;
    BranchGuard& operator=(const BranchGuard&) = delete;

private:
    bool condition_;
    bool iteration_;
};

// Name for the next loop, set by LoopName.
inline const char*& pendingLoopName()
{
    static thread_local const char* name = nullptr;
    return name;
}

// Takes the pending loop name, or the fallback.
inline const char* loopName(const char* fallback)
{
    const char*& pending = pendingLoopName();
    const char* name = pending ? pending : fallback;
    pending = nullptr;
    return name;
}

// Sets the name of the next loop (used for do-while).
class LoopName
{
public:
    explicit LoopName(const char* name) { pendingLoopName() = name; }

    operator int() const { return 0; }
};

// Reports a loop for as long as it lives.
class LoopGuard
{
public:
    explicit LoopGuard(const char* loopType) : type_(loopType) { Trackers::trackLoopEnter(type_); }

    ~LoopGuard() { Trackers::trackLoopExit(type_); }

    operator int() const { return 0; }

    LoopGuard(const LoopGuard&) = delete;
    LoopGuard& operator=(const LoopGuard&) = delete;

private:
    std::string type_;
};
}

#endif

// The macros below redefine if, for and while, so a program that includes this header is instrumented
// without rewriting those statements. There is no include guard around them on purpose: including the
// header again first removes the macros (InstrumentEnd.h) and then defines them anew. The instrumenter
// uses this to switch the macros off around raw regions such as system headers, and back on.
#include "InstrumentEnd.h"

// FV_UNIQ(a, b) pastes two tokens into one name. The extra level makes __COUNTER__ expand before the
// pasting, so every use gets a different name (two guards in one scope would otherwise clash).
#define FV_UNIQ_(a, b) a##b
#define FV_UNIQ(a, b) FV_UNIQ_(a, b)

// if: the condition becomes a declaration, if (BranchGuard guard{...}). The guard lives through the
// whole if/else, so its constructor reports the branch that is taken and its destructor reports the end.
// "" #__VA_ARGS__ turns the condition into a string literal (also when it is empty); __VA_ARGS__ keeps
// commas that are inside templates. (cond) ? true : false converts any condition type to bool.
#define FV_IF_(id, text, ...) \
    if (::fv::BranchGuard FV_UNIQ(fvBranch_, id){::fv::asCondition(((__VA_ARGS__) ? true : false), text)})
#define FV_IF(id, text, ...) FV_IF_(id, text, __VA_ARGS__)
#define if(...) FV_IF(__COUNTER__, "" #__VA_ARGS__, __VA_ARGS__)

// for: switch (LoopGuard guard{...}) case 0: for (...) if (IterationMark{}) {} else body
// - A declaration in a switch condition lives until the end of the switch, so the guard covers the
//   whole loop. LoopGuard converts to the int 0, which selects the one case 0 label; the switch only
//   exists to scope the guard.
// - IterationMark runs at the start of every pass. Its if is this header's own if macro: it sees
//   IterationMark, which sets iterationFlag and is false, so BranchGuard reports a loop pass instead
//   of a branch, and the else part (the user's body) always runs.
// - A macro is not expanded again inside its own expansion, so the for and while written in these
//   macros are the real keywords.
#define FV_FOR_(id, ...)                                                 \
    switch (::fv::LoopGuard FV_UNIQ(fvLoop_, id){::fv::loopName("for")}) \
    case 0:                                                              \
        for (__VA_ARGS__)                                                \
            if (::fv::IterationMark{}) {                                 \
            } else
// do-while: the instrumenter turns "do body while (cond);" into FV_DO_WHILE(cond) body. The for loop
// starts with go = true, so the body runs first, and go takes the value of cond after every pass.
// LoopName stores the name "do-while" for the LoopGuard that the for macro creates next.
#define FV_DO_WHILE_(id, ...)                                    \
    switch (::fv::LoopName FV_UNIQ(fvName_, id){"do-while"})     \
    case 0:                                                      \
        for (bool FV_UNIQ(fvGo_, id) = true; FV_UNIQ(fvGo_, id); \
             FV_UNIQ(fvGo_, id) = static_cast<bool>(__VA_ARGS__))
#define FV_DO_WHILE(...) FV_DO_WHILE_(__COUNTER__, __VA_ARGS__)
#define FV_FOR(id, ...) FV_FOR_(id, __VA_ARGS__)
#define for(...) FV_FOR(__COUNTER__, __VA_ARGS__)

// The while macro is left out when the program has a macro of its own that contains do ... while (such
// as do { ... } while (0)), which the while macro would turn into code that does not compile. The
// instrumenter then defines FV_NO_WHILE and while loops run normally but are not drawn.
#ifndef FV_NO_WHILE
#define FV_WHILE_(id, ...)                                                 \
    switch (::fv::LoopGuard FV_UNIQ(fvLoop_, id){::fv::loopName("while")}) \
    case 0:                                                                \
        while (__VA_ARGS__)                                                \
            if (::fv::IterationMark{}) {                                   \
            } else
#define FV_WHILE(id, ...) FV_WHILE_(id, __VA_ARGS__)
#define while(...) FV_WHILE(__COUNTER__, __VA_ARGS__)
#endif
