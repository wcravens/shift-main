#pragma once

#include <utility>

namespace shift::fix {

namespace details {

    template <typename _GroupType>
    auto createFIXGroupImpl(_GroupType& g) -> _GroupType&
    {
        return g;
    }

    template <typename _GroupType, typename FieldType, typename... _GroupItemTypes>
    auto createFIXGroupImpl(_GroupType& g, const FieldType& f, _GroupItemTypes&&... items) -> _GroupType&
    {
        g.setField(f);
        return createFIXGroupImpl(g, std::forward<_GroupItemTypes>(items)...);
    }

} // details

/**
 * @brief Create a specified type of FIX group and add all FIX field object 'items' into that.
 * @param _GroupType The type of FIX group.
 * @param items FIX field objects.
 */
template <typename _GroupType, typename... _GroupItemTypes>
auto createFIXGroup(_GroupItemTypes&&... items) -> _GroupType
{
    _GroupType g;
    return details::createFIXGroupImpl(g, std::forward<_GroupItemTypes>(items)...);
}

/**
 * @brief Create a specified type of FIX group and add all FIX field objects 'items' into that, add the group into the FIX message 'msg'.
 * @param _GroupType The type of FIX group.
 * @param msg FIX message.
 * @param item FIX field objects.
 */
template <typename _GroupType, typename _MsgType, typename... _GroupItemTypes>
void addFIXGroup(_MsgType& msg, _GroupItemTypes&&... items)
{
    msg.addGroup(createFIXGroup<_GroupType>(std::forward<_GroupItemTypes>(items)...));
}

namespace experimental {

    template <typename _MsgType>
    void getFIXField(const _MsgType& msg)
    {
        return;
    }

    template <typename _MsgType, typename _FieldType, typename... _FieldTypes>
    void getFIXField(const _MsgType& msg, _FieldType& f, _FieldTypes&... items)
    {
        msg.getField(f);
        getFIXField(msg, items...);
    }

    /** 
     * @brief Get fields from message.
     * @param msg FIX message.
     * @param items FIX field objects.
     */
    template <typename _MsgType, typename... _FieldTypes>
    void getFIXFields(const _MsgType& msg, _FieldTypes&... items)
    {
        getFIXField(msg, items...);
    }

    /**
         * @brief Get fields from one group inside the message.
         * @param noOfGroup The no. of group in the message.
         * @param msg FIX message.
         * @param items FIX field objects.
         */
    template <unsigned int noOfGroups, typename _GroupType, typename _MsgType, typename... _FieldTypes>
    void getFIXFields(const _MsgType& msg, _FieldTypes&... items)
    {
        _GroupType g;
        msg.getGroup(noOfGroups, g);
        getFIXField(g, items...);
    }

} // experiment

} // shift::fix
