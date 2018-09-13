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
    virtual Instrument* cloneImpl() const = 0; // clone pattern

public:
    Instrument(const Instrument&) = default; // copy constructor
    Instrument& operator=(const Instrument&) & = default; // copy assignment
    Instrument(Instrument&&) = default; // move constructor
    Instrument& operator=(Instrument&&) & = default; // move assignment
    virtual ~Instrument() = default; // virtual destructor

    Instrument(std::string symbol);

    std::unique_ptr<Instrument> clone() const; // clone pattern

    virtual Instrument::TYPE getType() const = 0;
    std::string getTypeString() const;

    const std::string& getSymbol() const;
};
