#pragma once

#include "Size.hpp"

class Bitmap;

class WatermarkSearchResult
{
public:
    void print(std::string const& handlerFCC, std::ostream& stream) const;

public:
    Size size_ { -1, -1 };
    std::shared_ptr<Bitmap> watermark_ = nullptr;
    int x_ = -1;
    int y_ = -1;
    int width_ = -1;
    int height_ = -1;
};
