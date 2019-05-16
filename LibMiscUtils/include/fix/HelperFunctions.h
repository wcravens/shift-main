#pragma once

#include <utility>

#include "../MiscUtils_EXPORTS.h"

namespace shift {
namespace fix {
    namespace details {
        template <typename _GroupType>
        _GroupType& createFIXGroupImpl(_GroupType& g)
        {
            return g;
        }

        template <typename _GroupType, typename FieldType, typename... _GroupItemTypes>
        _GroupType& createFIXGroupImpl(_GroupType& g, const FieldType& f, _GroupItemTypes&&... items)
        {
            g.setField(f);
            return createFIXGroupImpl(g, std::forward<_GroupItemTypes>(items)...);
        }
    } // namespace details

    template <typename _GroupType, typename... _GroupItemTypes>
    _GroupType createFIXGroup(_GroupItemTypes&&... items)
    {
        _GroupType g;
        return details::createFIXGroupImpl(g, std::forward<_GroupItemTypes>(items)...);
    }

    template <typename _GroupType, typename _MsgType, typename... _GroupItemTypes>
    void addFIXGroup(_MsgType& msg, _GroupItemTypes&&... items)
    {
        msg.addGroup(createFIXGroup<_GroupType>(std::forward<_GroupItemTypes>(items)...));
    }
} // fix
} // shift
