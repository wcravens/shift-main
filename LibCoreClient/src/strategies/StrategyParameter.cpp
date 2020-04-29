#include "strategies/StrategyParameter.h"

namespace shift::strategies {

//--------------------------------------------------------------------------

StrategyParameter::StrategyParameter()
    : m_tag { Tag::BOOL }
    , m_b { true }
{
}

StrategyParameter::~StrategyParameter()
{
}

//--------------------------------------------------------------------------

StrategyParameter::StrategyParameter(bool b)
    : m_tag { Tag::BOOL }
    , m_b { b }
{
}

StrategyParameter::StrategyParameter(char c)
    : m_tag { Tag::CHAR }
    , m_c { c }
{
}

StrategyParameter::StrategyParameter(double d)
    : m_tag { Tag::DOUBLE }
    , m_d { d }
{
}

StrategyParameter::StrategyParameter(int i)
    : m_tag { Tag::INT }
    , m_i { i }
{
}

StrategyParameter::StrategyParameter(std::string s)
    : m_tag { Tag::STRING }
    , m_s { std::move(s) }
{
}

StrategyParameter::StrategyParameter(const char* sl)
    : StrategyParameter { std::string(sl) }
{
}

StrategyParameter::StrategyParameter(const StrategyParameter& sp)
    : m_tag(sp.m_tag)
{
    if (Tag::BOOL == m_tag) {
        m_b = sp.m_b;
    } else if (Tag::CHAR == m_tag) {
        m_c = sp.m_c;
    } else if (Tag::DOUBLE == m_tag) {
        m_d = sp.m_d;
    } else if (Tag::INT == m_tag) {
        m_i = sp.m_i;
    } else if (Tag::STRING == m_tag) {
        new (&m_s) std::string(sp.m_s);
    }
}

//--------------------------------------------------------------------------

auto StrategyParameter::operator=(bool b) -> StrategyParameter&
{
    destroyString();

    m_b = b;
    m_tag = Tag::BOOL;

    return *this;
}

auto StrategyParameter::operator=(char c) -> StrategyParameter&
{
    destroyString();

    m_c = c;
    m_tag = Tag::CHAR;

    return *this;
}

auto StrategyParameter::operator=(double d) -> StrategyParameter&
{
    destroyString();

    m_d = d;
    m_tag = Tag::DOUBLE;

    return *this;
}

auto StrategyParameter::operator=(int i) -> StrategyParameter&
{
    destroyString();

    m_i = i;
    m_tag = Tag::INT;

    return *this;
}

auto StrategyParameter::operator=(const std::string& s) -> StrategyParameter&
{
    if (Tag::STRING == m_tag) {
        m_s = s;
    } else {
        new (&m_s) std::string(s);
        m_tag = Tag::STRING;
    }

    return *this;
}

auto StrategyParameter::operator=(const char* sl) -> StrategyParameter&
{
    *this = std::string(sl);

    return *this;
}

auto StrategyParameter::operator=(const StrategyParameter& sp) -> StrategyParameter&
{
    destroyString();

    m_tag = sp.m_tag;

    if (Tag::BOOL == m_tag) {
        m_b = sp.m_b;
    } else if (Tag::CHAR == m_tag) {
        m_c = sp.m_c;
    } else if (Tag::DOUBLE == m_tag) {
        m_d = sp.m_d;
    } else if (Tag::INT == m_tag) {
        m_i = sp.m_i;
    } else if (Tag::STRING == m_tag) {
        new (&m_s) std::string(sp.m_s);
    }

    return *this;
}

//--------------------------------------------------------------------------

auto StrategyParameter::isBool() const -> bool
{
    return Tag::BOOL == m_tag;
}

auto StrategyParameter::isChar() const -> bool
{
    return Tag::CHAR == m_tag;
}

auto StrategyParameter::isDouble() const -> bool
{
    return Tag::DOUBLE == m_tag;
}

auto StrategyParameter::isInt() const -> bool
{
    return Tag::INT == m_tag;
}

auto StrategyParameter::isString() const -> bool
{
    return Tag::STRING == m_tag;
}

//--------------------------------------------------------------------------

StrategyParameter::operator bool() const
{
    if (!isBool()) {
        throw new StrategyParameter::Invalid {};
    }

    return m_b;
}

StrategyParameter::operator char() const
{
    if (!isChar()) {
        throw new StrategyParameter::Invalid {};
    }

    return m_c;
}

StrategyParameter::operator double() const
{
    if (!isDouble() && !isInt()) {
        throw new StrategyParameter::Invalid {};
    }

    if (isInt()) {
        return m_i;
    }

    return m_d;
}

StrategyParameter::operator int() const
{
    if (!isInt()) {
        throw new StrategyParameter::Invalid {};
    }

    return m_i;
}

StrategyParameter::operator std::string() const
{
    if (!isString()) {
        throw new StrategyParameter::Invalid {};
    }

    return m_s;
}

//--------------------------------------------------------------------------

/* friend */ auto operator<<(std::ostream& os, StrategyParameter& sp) -> std::ostream&
{
    if (sp.isBool()) {
        return os << sp.m_b;
    }

    if (sp.isChar()) {
        return os << sp.m_c;
    }

    if (sp.isDouble()) {
        return os << sp.m_d;
    }

    if (sp.isInt()) {
        return os << sp.m_i;
    }

    if (sp.isString()) {
        return os << sp.m_s;
    }

    throw new StrategyParameter::Invalid {};
}

//--------------------------------------------------------------------------

auto StrategyParameter::destroyString() -> bool
{
    if (Tag::STRING == m_tag) {
        m_s.~basic_string<char>();
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------

} // shift::strategies
