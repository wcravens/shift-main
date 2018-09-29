#include "strategies/StrategyParameter.h"

namespace shift {
namespace strategies {

    // public:

    //--------------------------------------------------------------------------

    StrategyParameter::StrategyParameter()
        : m_tag(Tag::BOOL)
        , m_b(true)
    {
    }

    StrategyParameter::~StrategyParameter()
    {
    }

    //--------------------------------------------------------------------------

    StrategyParameter::StrategyParameter(bool b)
        : m_tag(Tag::BOOL)
        , m_b(b)
    {
    }

    StrategyParameter::StrategyParameter(char c)
        : m_tag(Tag::CHAR)
        , m_c(c)
    {
    }

    StrategyParameter::StrategyParameter(double d)
        : m_tag(Tag::DOUBLE)
        , m_d(d)
    {
    }

    StrategyParameter::StrategyParameter(int i)
        : m_tag(Tag::INT)
        , m_i(i)
    {
    }

    StrategyParameter::StrategyParameter(const std::string& s)
        : m_tag(Tag::STRING)
        , m_s(s)
    {
    }

    StrategyParameter::StrategyParameter(const char* sl)
        : StrategyParameter(std::string(sl))
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

    StrategyParameter& StrategyParameter::operator=(bool b)
    {
        destroyString();

        m_b = b;
        m_tag = Tag::BOOL;

        return *this;
    }

    StrategyParameter& StrategyParameter::operator=(char c)
    {
        destroyString();

        m_c = c;
        m_tag = Tag::CHAR;

        return *this;
    }

    StrategyParameter& StrategyParameter::operator=(double d)
    {
        destroyString();

        m_d = d;
        m_tag = Tag::DOUBLE;

        return *this;
    }

    StrategyParameter& StrategyParameter::operator=(int i)
    {
        destroyString();

        m_i = i;
        m_tag = Tag::INT;

        return *this;
    }

    StrategyParameter& StrategyParameter::operator=(const std::string& s)
    {
        if (Tag::STRING == m_tag) {
            m_s = s;
        } else {
            new (&m_s) std::string(s);
            m_tag = Tag::STRING;
        }

        return *this;
    }

    StrategyParameter& StrategyParameter::operator=(const char* sl)
    {
        *this = std::string(sl);

        return *this;
    }

    StrategyParameter& StrategyParameter::operator=(const StrategyParameter& sp)
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

    bool StrategyParameter::isBool() const
    {
        return Tag::BOOL == m_tag;
    }

    bool StrategyParameter::isChar() const
    {
        return Tag::CHAR == m_tag;
    }

    bool StrategyParameter::isDouble() const
    {
        return Tag::DOUBLE == m_tag;
    }

    bool StrategyParameter::isInt() const
    {
        return Tag::INT == m_tag;
    }

    bool StrategyParameter::isString() const
    {
        return Tag::STRING == m_tag;
    }

    //--------------------------------------------------------------------------

    StrategyParameter::operator bool() const
    {
        if (isBool())
            return m_b;
        else
            throw new StrategyParameter::Invalid{};
    }

    StrategyParameter::operator char() const
    {
        if (isChar())
            return m_c;
        else
            throw new StrategyParameter::Invalid{};
    }

    StrategyParameter::operator double() const
    {
        if (isDouble())
            return m_d;
        else if (isInt())
            return m_i;
        else
            throw new StrategyParameter::Invalid{};
    }

    StrategyParameter::operator int() const
    {
        if (isInt())
            return m_i;
        else
            throw new StrategyParameter::Invalid{};
    }

    StrategyParameter::operator std::string() const
    {
        if (isString())
            return m_s;
        else
            throw new StrategyParameter::Invalid{};
    }

    //--------------------------------------------------------------------------

    std::ostream& operator<<(std::ostream& os, StrategyParameter& sp)
    {
        if (sp.isBool()) {
            return os << sp.m_b;
        } else if (sp.isChar()) {
            return os << sp.m_c;
        } else if (sp.isDouble()) {
            return os << sp.m_d;
        } else if (sp.isInt()) {
            return os << sp.m_i;
        } else if (sp.isString()) {
            return os << sp.m_s;
        } else {
            throw new StrategyParameter::Invalid{};
        }
    }

    //--------------------------------------------------------------------------

    // private:

    //--------------------------------------------------------------------------

    bool StrategyParameter::destroyString()
    {
        if (Tag::STRING == m_tag) {
            m_s.~basic_string<char>();
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------

} // strategies
} // shift
