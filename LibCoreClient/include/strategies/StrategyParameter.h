#pragma once

#include <iostream>
#include <string>

namespace shift::strategies {

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
    StrategyParameter(std::string s);
    StrategyParameter(const char* sl);
    StrategyParameter(const StrategyParameter& sp);

    auto operator=(bool b) -> StrategyParameter&;
    auto operator=(char c) -> StrategyParameter&;
    auto operator=(double d) -> StrategyParameter&;
    auto operator=(int i) -> StrategyParameter&;
    auto operator=(const std::string& s) -> StrategyParameter&;
    auto operator=(const char* sl) -> StrategyParameter&;
    auto operator=(const StrategyParameter& sp) -> StrategyParameter&;

    auto isBool() const -> bool;
    auto isChar() const -> bool;
    auto isDouble() const -> bool;
    auto isInt() const -> bool;
    auto isString() const -> bool;

    operator bool() const;
    operator char() const;
    operator double() const;
    operator int() const;
    operator std::string() const;

    friend auto operator<<(std::ostream& os, StrategyParameter& sp) -> std::ostream&;

private:
    auto destroyString() -> bool;

    Tag m_tag;
    union {
        bool m_b;
        char m_c;
        double m_d;
        int m_i;
        std::string m_s;
    };
};

} // shift::strategies
