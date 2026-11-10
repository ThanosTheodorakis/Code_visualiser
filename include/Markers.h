// Markers.h - Markers, guards and handlers a program uses to report what it does.

#pragma once

#include <exception>
#include <memory>
#include <cctype>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define FUNCTION_ENTER(functionName) Trackers::trackFunctionEnter(functionName)
#define FUNCTION_EXIT(functionName) Trackers::trackFunctionExit(functionName)

#define BRANCH_ENTER(condition) Trackers::trackBranchEnter(condition)
#define BRANCH_EXIT(condition) Trackers::trackBranchExit(condition)

#define LOOP_ENTER(loopType) Trackers::trackLoopEnter(loopType)
#define LOOP_EXIT(loopType) Trackers::trackLoopExit(loopType)

#define BLOCK_ENTER(blockName) Trackers::trackBlockEnter(blockName)
#define BLOCK_EXIT(blockName) Trackers::trackBlockExit(blockName)

// Receives the events the markers report; every method does nothing unless overridden.
class TrackerHandler
{
public:
    virtual ~TrackerHandler() {}

    virtual void onFunctionEnter(const std::string&) {}

    virtual void onFunctionExit(const std::string&) {}

    virtual void onReturn(const std::string&) {}

    virtual void onBranchEnter(bool) {}

    virtual void onBranchEnter(bool condition, const std::string&) { onBranchEnter(condition); }

    virtual void onBranchExit(bool) {}

    virtual void onDecision(const std::string&, bool) {}

    virtual void onSwitchEnter(const std::string&, const std::string&) {}

    virtual void onSwitchExit() {}

    virtual void onLoopEnter(const std::string&) {}

    virtual void onLoopExit(const std::string&) {}

    virtual void onBlockEnter(const std::string&) {}

    virtual void onBlockExit(const std::string&) {}

    virtual void onLoopIteration(const std::string&, int) {}

    virtual void onStatement(const std::string&) {}

    virtual void onInput(const std::string&) {}

    virtual void onInputDone(const std::string&) {}

    virtual void onOutput(const std::string&) {}

    virtual void onProgramStart() {}

    virtual void onProgramEnd() {}

    virtual void onStop() {}
};

// Thrown by a marker when the user closes the window, to end the program.
struct StopRequested : public std::exception
{
    const char* what() const noexcept override { return "stop requested"; }
};

// Static functions the markers call; each forwards its event to the registered handlers.
struct Trackers
{
    // A function call starts.
    static void trackFunctionEnter(const std::string& functionName);
    // A function call ends.
    static void trackFunctionExit(const std::string& functionName);
    // A function returns the given value.
    static void trackReturn(const std::string& value);

    // An if statement is entered; condition tells which branch is taken.
    static void trackBranchEnter(bool condition, const std::string& text = "");
    // An if statement ends.
    static void trackBranchExit(bool condition);

    // One part of a combined condition (&&, || or ?:) was evaluated.
    static void trackDecision(const std::string& text, bool value);
    // A switch starts with its expression and value.
    static void trackSwitchEnter(const std::string& expression, const std::string& value);
    // A switch ends.
    static void trackSwitchExit();

    // A loop starts.
    static void trackLoopEnter(const std::string& loopType);
    // A loop ends.
    static void trackLoopExit(const std::string& loopType);

    // A loop pass starts; counted per loop.
    static void trackLoopIteration();

    // A statement is about to run.
    static void trackStatement(const std::string& text);

    // The program starts reading input.
    static void trackInput(const std::string& text);
    // The input has been read.
    static void trackInputDone(const std::string& text);
    // The program writes output.
    static void trackOutput(const std::string& text);

    // The program starts.
    static void trackProgramStart();
    // The program ends.
    static void trackProgramEnd();

    // A named block starts.
    static void trackBlockEnter(const std::string& blockName);
    // A named block ends.
    static void trackBlockExit(const std::string& blockName);

    // True once a stop was requested.
    static bool stopRequested();
    // Asks the program to stop at its next marker.
    static void requestStop();
    // Clears the stop request.
    static void resetStop();

    // Registers a handler.
    static void addHandler(std::shared_ptr<TrackerHandler> handler);
    // Removes all handlers.
    static void clearHandlers();

    // Prints every event to the console.
    static void installConsoleHandler();
};

namespace fv {
// Text of a value that can be streamed.
template <class T>
auto describeValue(const T& value, int) -> decltype(std::declval<std::ostream&>() << value, std::string())
{
    std::ostringstream out;
    out << value;
    return out.str();
}

// Fallback for values that cannot be streamed.
template <class T> std::string describeValue(const T&, long)
{
    return "?";
}

inline std::string describeValue(bool value, int)
{
    return value ? "true" : "false";
}

inline std::string describeValue(const std::string& value, int)
{
    return "\"" + value + "\"";
}

inline std::string describeValue(const char* value, int)
{
    return value ? "\"" + std::string(value) + "\"" : "null";
}

inline std::string describeValue(char* value, int)
{
    return describeValue(static_cast<const char*>(value), 0);
}

inline std::string describeValue(char value, int)
{
    return std::string("'") + value + "'";
}

// True while a value is being turned into text; markers stay silent then.
inline bool& describing()
{
    static thread_local bool flag = false;
    return flag;
}

// Sets describing() for the lifetime of the object.
struct DescribeScope
{
    bool previous;

    DescribeScope() : previous(describing()) { describing() = true; }

    ~DescribeScope() { describing() = previous; }
};

// Short text of a value (at most 24 characters), or "?" if already describing one.
template <class T> std::string valueText(const T& value)
{
    if (describing()) {
        return "?";
    }
    DescribeScope scope;
    std::string text = describeValue(value, 0);
    if (text.size() > 24) {
        text = text.substr(0, 21) + "...";
    }
    return text;
}

// Splits "a, b, c" at top-level commas into count names.
inline void splitNames(const char* names, std::string* out, int count)
{
    int index = 0;
    int depth = 0;
    std::string current;
    for (const char* c = names; *c && index < count; ++c) {
        if (*c == '(' || *c == '[' || *c == '{') {
            ++depth;
        }
        if (*c == ')' || *c == ']' || *c == '}') {
            --depth;
        }
        if (*c == ',' && depth == 0) {
            out[index++] = current;
            current.clear();
        } else if (!(current.empty() && *c == ' ')) {
            current += *c;
        }
    }
    if (index < count) {
        out[index] = current;
    }
}

// Builds "name=value, ..." for a function entry.
struct ArgsOf
{
    const char* names;

    template <class... Args> std::string operator()(const Args&... values) const
    {
        const int count = static_cast<int>(sizeof...(Args));
        std::vector<std::string> parts(count + 1);
        splitNames(names, parts.data(), count);
        const std::string texts[] = {valueText(values)..., std::string()};
        std::string result;
        for (int i = 0; i < count; ++i) {
            if (i > 0) {
                result += ", ";
            }
            result += parts[i] + "=" + texts[i];
        }
        return result;
    }
};

// Creates an ArgsOf for a list of argument names.
inline ArgsOf argsOf(const char* names)
{
    return ArgsOf{names};
}

// Reports a returned value and passes it on.
template <class T> T&& returned(T&& value)
{
    Trackers::trackReturn(valueText(value));
    return std::forward<T>(value);
}

// Removes the FV_COND, FV_INPUT, FV_OUTPUT and FV_RETURN wrappers from source text.
inline std::string cleanSource(const std::string& text)
{
    static const char* const wrappers[] = {"FV_COND(", "FV_INPUT(", "FV_OUTPUT(", "FV_RETURN("};
    std::string out;
    std::vector<int> dropped;
    int depth = 0;
    for (std::size_t i = 0; i < text.size();) {
        bool matched = false;
        const bool wordStart =
            i == 0 || !(std::isalnum(static_cast<unsigned char>(text[i - 1])) || text[i - 1] == '_');
        for (const char* wrapper : wrappers) {
            const std::size_t length = std::char_traits<char>::length(wrapper);
            if (wordStart && text.compare(i, length, wrapper) == 0) {
                ++depth;
                dropped.push_back(depth);
                i += length;
                matched = true;
                break;
            }
        }
        if (matched) {
            continue;
        }
        const char c = text[i++];
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            if (!dropped.empty() && dropped.back() == depth) {
                dropped.pop_back();
                --depth;
                continue;
            }
            --depth;
        }
        out += c;
    }
    return out;
}

// Reports the start and end of a switch; converts to the switched value.
template <class T> class SwitchGuard
{
public:
    SwitchGuard(const char* expression, T value) : value_(value), armed_(false)
    {
        Trackers::trackSwitchEnter(cleanSource(expression), valueText(value));
        armed_ = true;
    }

    SwitchGuard(SwitchGuard&& other) : value_(other.value_), armed_(other.armed_) { other.armed_ = false; }

    ~SwitchGuard()
    {
        if (armed_) {
            Trackers::trackSwitchExit();
        }
    }

    operator T() const { return value_; }

    SwitchGuard(const SwitchGuard&) = delete;
    SwitchGuard& operator=(const SwitchGuard&) = delete;

private:
    T value_;
    bool armed_;
};

// Creates the guard of a switch.
template <class T> SwitchGuard<T> switchOn(const char* expression, T value)
{
    return SwitchGuard<T>(expression, value);
}

// Reports one condition part and returns its value.
inline bool decision(const char* text, bool value)
{
    Trackers::trackDecision(cleanSource(text), value);
    return value;
}
}

// Reports a function call for as long as it lives.
class FunctionScope
{
public:
    explicit FunctionScope(const char* name, const std::string& arguments = "") : name_(name)
    {
        Trackers::trackFunctionEnter(arguments.empty() ? name_ : name_ + "(" + arguments + ")");
    }

    ~FunctionScope() { Trackers::trackFunctionExit(name_); }

    FunctionScope(const FunctionScope&) = delete;
    FunctionScope& operator=(const FunctionScope&) = delete;

private:
    std::string name_;
};

// Reports an if branch for as long as it lives.
class BranchScope
{
public:
    explicit BranchScope(bool condition) : condition_(condition) { Trackers::trackBranchEnter(condition_); }

    ~BranchScope() { Trackers::trackBranchExit(condition_); }

    BranchScope(const BranchScope&) = delete;
    BranchScope& operator=(const BranchScope&) = delete;

private:
    bool condition_;
};

// Reports a loop for as long as it lives.
class LoopScope
{
public:
    explicit LoopScope(const char* loopType) : type_(loopType) { Trackers::trackLoopEnter(type_); }

    ~LoopScope() { Trackers::trackLoopExit(type_); }

    LoopScope(const LoopScope&) = delete;
    LoopScope& operator=(const LoopScope&) = delete;

private:
    std::string type_;
};

// Reports the start and end of the program.
class ProgramScope
{
public:
    ProgramScope() { Trackers::trackProgramStart(); }

    ~ProgramScope() { Trackers::trackProgramEnd(); }

    ProgramScope(const ProgramScope&) = delete;
    ProgramScope& operator=(const ProgramScope&) = delete;
};

// FV_CONCAT pastes two tokens; the extra level makes __COUNTER__ expand first so each name is unique.
#define FV_CONCAT_INNER(a, b) a##b
#define FV_CONCAT(a, b) FV_CONCAT_INNER(a, b)

#define FUNCTION_SCOPE(functionName) FunctionScope FV_CONCAT(fvFunctionScope_, __COUNTER__)(functionName)

// Reports a function call for the rest of the function body. __func__ is the function's name; argsOf
// splits the argument names so the entry shows name=value.
#define FV_FUNCTION(...)                                             \
    FunctionScope FV_CONCAT(fvFunctionScope_, __COUNTER__)(__func__, \
                                                           ::fv::argsOf("" #__VA_ARGS__)(__VA_ARGS__))

// Reports the returned value and passes it on unchanged.
#define FV_RETURN(...) (::fv::returned(__VA_ARGS__))

// Statement markers: a comma expression that reports the statement first and then evaluates it, so the
// value and the evaluation order do not change. FV_STMT_AS uses the given text, FV_DECL_AS is for
// declarations (nothing is evaluated).
#define FV_STMT_AS(text, ...) (::Trackers::trackStatement(text), (__VA_ARGS__))

#define FV_DECL_AS(text) ::Trackers::trackStatement(text)

// One part of a combined condition (&&, ||, ?:). It reports the text and value of the part and returns the
// value, so the short-circuit behaviour of the whole condition stays the same.
#define FV_COND(...) (::fv::decision(#__VA_ARGS__, ((__VA_ARGS__) ? true : false)))

// The guard is declared in the switch condition, so it lives for the whole switch (its destructor reports
// the end) and converts to the switched value.
#define FV_SWITCH(...) \
    switch (auto FV_CONCAT(fvSwitch_, __COUNTER__) = ::fv::switchOn("" #__VA_ARGS__, (__VA_ARGS__)))
#define BRANCH_SCOPE(condition) BranchScope FV_CONCAT(fvBranchScope_, __COUNTER__)(condition)
#define LOOP_SCOPE(loopType) LoopScope FV_CONCAT(fvLoopScope_, __COUNTER__)(loopType)

#define FV_STMT(...) (::Trackers::trackStatement(#__VA_ARGS__), (__VA_ARGS__))

namespace fv {
// Reports the start of an input and, when destroyed, its end.
class InputMark
{
public:
    explicit InputMark(const char* text) : text_(text) { Trackers::trackInput(text_); }

    ~InputMark() { Trackers::trackInputDone(text_); }

    InputMark(const InputMark&) = delete;
    InputMark& operator=(const InputMark&) = delete;

private:
    std::string text_;
};
}

// The InputMark temporary lives until the end of the full expression: it reports the start before the
// input is read and the end after it.
#define FV_INPUT(...) ((void)::fv::InputMark(#__VA_ARGS__), (__VA_ARGS__))

#define FV_OUTPUT(...) (::Trackers::trackOutput(#__VA_ARGS__), (__VA_ARGS__))

#define FV_ITERATION() Trackers::trackLoopIteration()
