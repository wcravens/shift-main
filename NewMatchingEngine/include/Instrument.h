#pragma once

#include <iostream>
#include <memory>
#include <string>

class Instrument {

    ///////////////////////////////////////////////////////////////////////////////
public:
    enum class TYPE {
        COMMON_STOCK,
        PREFERRED_STOCK,
    };

    struct Comparator {
        bool operator()(const std::unique_ptr<Instrument>& a, const std::unique_ptr<Instrument>& b) const
        {
            return a->getSymbol() < b->getSymbol();
        }
    };

private:
    static std::string instrumentTypeToString(Instrument::TYPE type)
    {
        switch (type) {
        case Instrument::TYPE::COMMON_STOCK:
            return "CS";
        case Instrument::TYPE::PREFERRED_STOCK:
            return "PS";
        default:
            return "[Unknown Instrument::TYPE]";
        }
    }
    ///////////////////////////////////////////////////////////////////////////////

protected:
    std::string m_symbol;

protected:
    virtual auto cloneImpl() const -> Instrument* = 0; // clone pattern

public:
    Instrument(const Instrument&) = default; // copy constructor
    auto operator=(const Instrument&) & -> Instrument& = default; // lvalue-only copy assignment
    Instrument(Instrument&&) = default; // move constructor
    auto operator=(Instrument&&) & -> Instrument& = default; // lvalue-only move assignment
    virtual ~Instrument() = default; // virtual destructor

    Instrument(std::string symbol);

    auto clone() const -> std::unique_ptr<Instrument>; // clone pattern

    virtual auto getType() const -> Instrument::TYPE = 0;
    auto getTypeString() const -> std::string;

    auto getSymbol() const -> const std::string&;
};
