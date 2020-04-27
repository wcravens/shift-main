#pragma once

#include "Instrument.h"

#include <string>

class CommonStock : public Instrument {

protected:
    virtual auto cloneImpl() const -> CommonStock* override; // clone pattern

public:
    CommonStock(const CommonStock&) = default; // copy constructor
    auto operator=(const CommonStock&) & -> CommonStock& = default; // lvalue-only copy assignment
    CommonStock(CommonStock&&) = default; // move constructor
    auto operator=(CommonStock&&) & -> CommonStock& = default; // lvalue-only move assignment
    virtual ~CommonStock() = default; // virtual destructor

    CommonStock(std::string symbol);

    virtual auto getType() const -> Instrument::TYPE override;
};
