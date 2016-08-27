#pragma once

class Colorspace
{
public:
    static void getYCbCrFromRGB(float r, float g, float b, short& y, short& cb, short&cr)
    {
        float const Y = kR * r + (1 - kR - kB) * g + kB * b;
        float const Cr = (r - Y) / (2 * (1 - kR));
        float const Cb = (b - Y) / (2 * (1 - kB));

        y = (short)(Y * 4096);
        cr = (short)(Cr * 2048);
        cb = (short)(Cb * 2048);
    }

    static void getRGBFromYCbCr(short y, short cb, short cr, float& r, float& g, float& b)
    {
        float const Y = y / 4096.0f;
        float const Cr = cr / 2048.0f;
        float const Cb = cb / 2048.0f;

        r = Y + 2 * Cr * (1 - kR);
        b = Y + 2 * Cb * (1 - kB);
        g = (Y - kR * r - kB * b) / (1 - kR - kB);
    }

private:
    static constexpr float kR = 0.299f;
    static constexpr float kG = 0.587f;
    static constexpr float kB = 0.114f;
};
