#pragma once

#include <iostream>
#include <string>

namespace shift {
namespace strategies {

    class StrategyParameter {

    public:
        class Invalid {
        };

    private:
        enum class Tag {
            BOOL,
            CHAR,
            DOUBLE,
            INT,
            STRING,
        };

    public:
        StrategyParameter();
        ~StrategyParameter();

        StrategyParameter(bool b);
        StrategyParameter(char c);
        StrategyParameter(double d);
        StrategyParameter(int i);
        StrategyParameter(const std::string& s);
        StrategyParameter(const char* sl);
        StrategyParameter(const StrategyParameter& sp);

        StrategyParameter& operator=(bool b);
        StrategyParameter& operator=(char c);
        StrategyParameter& operator=(double d);
        StrategyParameter& operator=(int i);
        StrategyParameter& operator=(const std::string& s);
        StrategyParameter& operator=(const char* sl);
        StrategyParameter& operator=(const StrategyParameter& sp);

        bool isBool() const;
        bool isChar() const;
        bool isDouble() const;
        bool isInt() const;
        bool isString() const;

        operator bool() const;
        operator char() const;
        operator double() const;
        operator int() const;
        operator std::string() const;

        friend std::ostream& operator<<(std::ostream& os, StrategyParameter& sp);

    private:
        bool destroyString();

        Tag m_tag;
        union {
            bool m_b;
            char m_c;
            double m_d;
            int m_i;
            std::string m_s;
        };
    };

} // strategies
} // shift
