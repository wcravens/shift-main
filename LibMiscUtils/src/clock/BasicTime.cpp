#include "clock/BasicTime.h"

shift::clock::BasicTime::BasicTime()
    : sec{ time(nullptr) }
    , usec{ 0 }
{
}

bool shift::clock::BasicTime::operator==(const BasicTime& bt)
{
    return (this->sec == bt.sec) && (this->usec == bt.usec);
}

bool shift::clock::BasicTime::operator==(const BasicTime& bt) const
{
    return (this->sec == bt.sec) && (this->usec == bt.usec);
}

bool shift::clock::BasicTime::operator<(const BasicTime& bt)
{
    if (this->sec < bt.sec) {
        return true;
    } else if (this->sec == bt.sec) {
        if (this->usec < bt.usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::BasicTime::operator<(const BasicTime& bt) const
{
    if (this->sec < bt.sec) {
        return true;
    } else if (this->sec == bt.sec) {
        if (this->usec < bt.usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::BasicTime::operator<=(const BasicTime& bt)
{
    return !(*this > bt);
}

bool shift::clock::BasicTime::operator<=(const BasicTime& bt) const
{
    return !(*this > bt);
}

bool shift::clock::BasicTime::operator>(const BasicTime& bt)
{
    if (this->sec > bt.sec) {
        return true;
    } else if (this->sec == bt.sec) {
        if (this->usec > bt.usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::BasicTime::operator>(const BasicTime& bt) const
{
    if (this->sec > bt.sec) {
        return true;
    } else if (this->sec == bt.sec) {
        if (this->usec > bt.usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::BasicTime::operator>=(const BasicTime& bt)
{
    return !(*this < bt);
}

bool shift::clock::BasicTime::operator>=(const BasicTime& bt) const
{
    return !(*this < bt);
}