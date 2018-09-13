#pragma once

#include "Instrument.h"

#include <string>

class CommonStock : public Instrument {

protected:
    CommonStock* cloneImpl() const override; // clone pattern

public:
    CommonStock(const CommonStock&) = default; // copy constructor
    CommonStock& operator=(const CommonStock&) & = default; // copy assignment
    CommonStock(CommonStock&&) = default; // move constructor
    CommonStock& operator=(CommonStock&&) & = default; // move assignment
    virtual ~CommonStock() = default; // virtual destructor

    CommonStock(std::string symbol);

    Instrument::TYPE getType() const override;
};
