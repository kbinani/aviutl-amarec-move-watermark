#pragma once

#include "MaskPixel.h"

class Mask
{
public:
    Mask(int width, int height, MaskPixel const* pixelData, float alpha)
        : width_(width)
        , height_(height)
        , pixelData_(pixelData)
        , alpha_(alpha)
    {}

    void unmask(MaskPixel const* mask, float r, float g, float b, float& outR, float& outG, float& outB) const
    {
        outR = (r - mask->fr * alpha_) / (1 - alpha_);
        outG = (g - mask->fg * alpha_) / (1 - alpha_);
        outB = (b - mask->fb * alpha_) / (1 - alpha_);
    }

    void mask(MaskPixel const* mask, float r, float g, float b, float& outR, float& outG, float& outB) const
    {
        outR = mask->fr * alpha_ + r * (1 - alpha_);
        outG = mask->fg * alpha_ + g * (1 - alpha_);
        outB = mask->fb * alpha_ + b * (1 - alpha_);
    }

    static Mask const* selectMask(int frameWidth, int frameHeight, int& leftMargin, int& bottomMargin);

public:
    int const width_;
    int const height_;
    MaskPixel const* const pixelData_;
    float const alpha_;
};
