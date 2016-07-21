#pragma once

#include "Color.hpp"

class Bitmap
{
public:
    Bitmap(size_t width, size_t height);

    ~Bitmap();

    void save(std::string const& file, std::wstring const& mimeType) const;

    std::string md5() const;

    void setPixel(size_t x, size_t y, Color color);

    Color pixel(size_t x, size_t y) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
