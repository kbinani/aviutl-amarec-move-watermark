#pragma once

#include "Size.hpp"

class SizeComparator
{
public:
    bool operator()(Size const& a, Size const& b) const
    {
        if (a.width_ < b.width_) {
            return true;
        } else if (a.width_ == b.width_) {
            return (a.height_ < b.height_);
        } else {
            return false;
        }
    }
};
