#include "WatermarkSearchResult.hpp"
#include "Bitmap.hpp"

void WatermarkSearchResult::print(std::string const& handlerFCC, std::ostream& stream) const
{
    stream << size_.width_ << "\t" << size_.height_;
    if (watermark_ == nullptr) {
        stream << std::endl;
    } else {
        stream << "\t" << x_ << "\t" << y_ << "\t" << width_ << "\t" << height_ << std::endl;

        std::string filename = handlerFCC + std::string("_") + std::to_string(width_) + std::string("x") + std::to_string(height_) + std::string("_") + watermark_->md5() + std::string(".png");
        if (!std::tr2::sys::exists(filename)) {
            watermark_->save(filename, L"image/png");
        }
    }
}
