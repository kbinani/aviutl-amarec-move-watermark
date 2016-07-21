#pragma once

class Color
{
public:
    Color()
    {
    }

    Color(uint8_t red, uint8_t green, uint8_t blue)
        : red_(red)
        , green_(green)
        , blue_(blue)
    {}

    uint8_t red_ = 0;
    uint8_t green_ = 0;
    uint8_t blue_ = 0;
};
