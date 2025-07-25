/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_LOG_H_
#define _FCITX_UTILS_LOG_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Log utilities.

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/tuplehelpers.h>
#include <source_location> // IWYU pragma: keep
#include <span>

namespace fcitx {

/// \brief LogLevel from high to low.
enum LogLevel : int {
    NoLog = 0,
    /// Fatal will always abort regardless of log or not.
    Fatal = 1,
    Error = 2,
    Warn = 3,
    Info = 4,
    Debug = 5,
    LastLogLevel = Debug
};

#define FCITX_SIMPLE_LOG(TYPE)                                                 \
    inline LogMessageBuilder &operator<<(TYPE v) {                             \
        out_ << v;                                                             \
        return *this;                                                          \
    }

class LogCategoryPrivate;
class FCITXUTILS_EXPORT LogCategory {
public:
    LogCategory(const char *name, LogLevel level = LogLevel::Info);
    ~LogCategory();

    LogLevel logLevel() const;
    bool checkLogLevel(LogLevel l) const;
    void setLogLevel(LogLevel l);
    void setLogLevel(std::underlying_type_t<LogLevel> l);
    void resetLogLevel();
    const std::string &name() const;

    // Helper function
    bool fatalWrapper(LogLevel l) const;
    static bool fatalWrapper2(LogLevel l);

private:
    FCITX_DECLARE_PRIVATE(LogCategory);
    std::unique_ptr<LogCategoryPrivate> d_ptr;
};

class FCITXUTILS_EXPORT Log {
public:
    static const LogCategory &defaultCategory();
    static void setLogRule(const std::string &rule);
    /**
     * @brief set the global log stream to be used by default.
     *
     * This function is not thread safe.
     * Please ensure there is no other thread using it at the same time.
     * By default is std::cerr. When you pass a stream into it, you need to
     * ensure it out-live the last call to the function. You may reset it to
     * std::cerr when you don't want to keep the old stream anymore.
     *
     * @param stream
     * @since 5.0.6
     */
    static void setLogStream(std::ostream &stream);
    /**
     * @brief Return the default log stream to be used.
     *
     * @return std::ostream&
     * @since 5.0.6
     */
    static std::ostream &logStream();
};

class FCITXUTILS_EXPORT LogMessageBuilder {
public:
    LogMessageBuilder(std::ostream &out, LogLevel l, const char *filename,
                      int lineNumber);
    ~LogMessageBuilder();

    LogMessageBuilder &self() { return *this; }

    LogMessageBuilder &operator<<(const std::string &s) {
        *this << s.c_str();
        return *this;
    }

    LogMessageBuilder &operator<<(const Key &key) {
        out_ << "Key(" << key.toString()
             << " states=" << key.states().toInteger() << ")";
        return *this;
    }

    FCITX_SIMPLE_LOG(char)
    FCITX_SIMPLE_LOG(bool)
    FCITX_SIMPLE_LOG(signed short)
    FCITX_SIMPLE_LOG(unsigned short)
    FCITX_SIMPLE_LOG(signed int)
    FCITX_SIMPLE_LOG(unsigned int)
    FCITX_SIMPLE_LOG(signed long)
    FCITX_SIMPLE_LOG(unsigned long)
    FCITX_SIMPLE_LOG(float)
    FCITX_SIMPLE_LOG(double)
    FCITX_SIMPLE_LOG(char *)
    FCITX_SIMPLE_LOG(const char *)
    FCITX_SIMPLE_LOG(const void *)
    FCITX_SIMPLE_LOG(long double)
    FCITX_SIMPLE_LOG(signed long long)
    FCITX_SIMPLE_LOG(unsigned long long)

    // For some random type, use ostream.
    template <typename T>
    FCITX_SIMPLE_LOG(T)

    template <typename T>
    LogMessageBuilder &operator<<(const std::optional<T> &opt) {
        *this << "optional(has_value=" << opt.has_value() << " ";
        if (opt.has_value()) {
            *this << *opt;
        }
        *this << ")";
        return *this;
    }

    template <typename T>
    LogMessageBuilder &operator<<(const std::unique_ptr<T> &ptr) {
        *this << "unique_ptr(" << ptr.get() << ")";
        return *this;
    }

    template <typename T>
    LogMessageBuilder &operator<<(const std::vector<T> &vec) {
        *this << "[";
        printRange(vec.begin(), vec.end());
        *this << "]";
        return *this;
    }

    template <typename T>
    LogMessageBuilder &operator<<(const std::span<T> &vec) {
        *this << "span[";
        printRange(vec.begin(), vec.end());
        *this << "]";
        return *this;
    }

    template <typename T>
    LogMessageBuilder &operator<<(const std::list<T> &lst) {
        *this << "list[";
        printRange(lst.begin(), lst.end());
        *this << "]";
        return *this;
    }

    template <typename K, typename V>
    LogMessageBuilder &operator<<(const std::pair<K, V> &pair) {
        *this << "(" << pair.first << ", " << pair.second << ")";
        return *this;
    }

    template <typename... Args>
    LogMessageBuilder &operator<<(const std::tuple<Args...> &tuple) {
        typename MakeSequence<sizeof...(Args)>::type a;
        *this << "(";
        printWithIndices(a, tuple);
        *this << ")";
        return *this;
    }

    template <typename K, typename V>
    LogMessageBuilder &operator<<(const std::unordered_map<K, V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename V>
    LogMessageBuilder &operator<<(const std::unordered_set<V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename K, typename V>
    LogMessageBuilder &operator<<(const std::map<K, V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename V>
    LogMessageBuilder &operator<<(const std::set<V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename K, typename V>
    LogMessageBuilder &operator<<(const std::multimap<K, V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename V>
    LogMessageBuilder &operator<<(const std::multiset<V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename K, typename V>
    LogMessageBuilder &operator<<(const std::unordered_multimap<K, V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename V>
    LogMessageBuilder &operator<<(const std::unordered_multiset<V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

private:
    template <typename Iterator>
    void printRange(Iterator begin, Iterator end) {
        bool first = true;
        for (auto &item : MakeIterRange(begin, end)) {
            if (first) {
                first = false;
            } else {
                *this << ", ";
            }
            *this << item;
        }
    }

    template <typename... Args, int... S>
    void printWithIndices(Sequence<S...> /*unused*/,
                          const std::tuple<Args...> &tuple) {
        using swallow = int[];
        (void)swallow{
            0,
            (void(*this << (S == 0 ? "" : ", ") << std::get<S>(tuple)), 0)...};
    }

    std::ostream &out_;
};

template <typename MetaStringFileName, int N>
class LogMessageBuilderWrapper {
public:
    LogMessageBuilderWrapper(LogLevel l)
        : builder_(Log::logStream(), l, MetaStringFileName::data(), N) {}

    LogMessageBuilder &self() { return builder_; }

private:
    LogMessageBuilder builder_;
};

} // namespace fcitx

// Use meta string for file name to avoid having full path in binary.
#define FCITX_LOGC_IF(CATEGORY, LEVEL, CONDITION)                              \
    for (bool fcitxLogEnabled =                                                \
             (CONDITION) && CATEGORY().fatalWrapper(::fcitx::LogLevel::LEVEL); \
         fcitxLogEnabled;                                                      \
         fcitxLogEnabled = CATEGORY().fatalWrapper2(::fcitx::LogLevel::LEVEL)) \
    ::fcitx::LogMessageBuilderWrapper<                                         \
        fcitx::MetaStringBasenameType<fcitxMakeMetaString(                     \
            std::source_location::current().file_name())>,                     \
        std::source_location::current().line()>(::fcitx::LogLevel::LEVEL)      \
        .self()

#define FCITX_LOGC(CATEGORY, LEVEL)                                            \
    for (bool fcitxLogEnabled =                                                \
             CATEGORY().fatalWrapper(::fcitx::LogLevel::LEVEL);                \
         fcitxLogEnabled;                                                      \
         fcitxLogEnabled = CATEGORY().fatalWrapper2(::fcitx::LogLevel::LEVEL)) \
    ::fcitx::LogMessageBuilderWrapper<                                         \
        fcitx::MetaStringBasenameType<fcitxMakeMetaString(                     \
            std::source_location::current().file_name())>,                     \
        std::source_location::current().line()>(::fcitx::LogLevel::LEVEL)      \
        .self()

#define FCITX_LOG(LEVEL) FCITX_LOGC(::fcitx::Log::defaultCategory, LEVEL)

#define FCITX_DEBUG() FCITX_LOG(Debug)
#define FCITX_WARN() FCITX_LOG(Warn)
#define FCITX_INFO() FCITX_LOG(Info)
#define FCITX_ERROR() FCITX_LOG(Error)
#define FCITX_FATAL() FCITX_LOG(Fatal)

#define FCITX_LOG_IF(LEVEL, CONDITION)                                         \
    FCITX_LOGC_IF(::fcitx::Log::defaultCategory, LEVEL, CONDITION)

#define FCITX_ASSERT(...)                                                      \
    FCITX_LOG_IF(Fatal, !(__VA_ARGS__)) << #__VA_ARGS__ << " failed. "

#define FCITX_DEFINE_LOG_CATEGORY(name, ...)                                   \
    const ::fcitx::LogCategory &name() {                                       \
        static const ::fcitx::LogCategory category(__VA_ARGS__);               \
        return category;                                                       \
    }

#define FCITX_DECLARE_LOG_CATEGORY(name) const ::fcitx::LogCategory &name()

#endif // _FCITX_UTILS_LOG_H_
