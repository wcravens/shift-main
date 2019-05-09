#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <map>

namespace shift {
namespace fix {
    template <typename GroupType>
    GroupType createFIXGroup(GroupType g)
    {
        return g;
    }

    template <typename GroupType, typename FieldType, typename... Args>
    GroupType createFIXGroup(GroupType g, FieldType f, Args... args)
    {
        g.setField(f);
        return createFIXGroup(g, args...);
    }

    template <typename GroupType, typename... Args>
    GroupType createFIXGroup(Args... args)
    {
        GroupType g;
        return createFIXGroup(g, args...);
    }

} // fix
} // shift
