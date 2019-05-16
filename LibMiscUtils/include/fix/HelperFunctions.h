#pragma once

#include <utility>

#include "../MiscUtils_EXPORTS.h"

namespace shift {
namespace fix {
    namespace details {
        template <typename GroupType>
        GroupType createFIXGroupImpl(GroupType& g)
        {
            return g;
        }

        template <typename GroupType, typename FieldType, typename... Args>
        GroupType createFIXGroupImpl(GroupType& g, const FieldType& f, Args&&... args)
        {
            g.setField(f);
            return createFIXGroupImpl(g, std::forward<Args>(args)...);
        }
    } // namespace details

    template <typename GroupType, typename... Args>
    GroupType createFIXGroup(Args&&... args)
    {
        GroupType g;
        return details::createFIXGroupImpl(g, std::forward<Args>(args)...);
    }

    template <typename GroupType, typename MsgType, typename... Args>
    void addFIXGroup(MsgType& msg, Args&&... args)
    {
        msg.addGroup(createFIXGroup<GroupType>(std::forward<Args>(args)...));
    }

} // fix
} // shift
