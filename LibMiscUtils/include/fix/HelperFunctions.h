#pragma once

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
        GroupType createFIXGroupImpl(GroupType& g, FieldType f, Args... args)
        {
            g.setField(f);
            return createFIXGroupImpl(g, args...);
        }
    } // namespace details

    template <typename GroupType, typename... Args>
    GroupType createFIXGroup(Args... args)
    {
        GroupType g;
        return details::createFIXGroupImpl(g, args...);
    }

    template <typename GroupType, typename MsgType, typename... Args>
    void addFIXGroup(MsgType& msg, Args... args)
    {
        msg.addGroup(createFIXGroup<GroupType>(args...));
    }

} // fix
} // shift
