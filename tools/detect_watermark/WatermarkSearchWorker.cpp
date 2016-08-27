#include "WatermarkSearchWorker.hpp"
#include "WatermarkSearchResult.hpp"
#include "Bitmap.hpp"

WatermarkSearchWorker::WatermarkSearchWorker(std::string const& handlerFCC)
    : compressor_(nullptr)
    , decompressor_(nullptr)
    , handlerFCC_(handlerFCC)
{}


void WatermarkSearchWorker::run()
{
    DWORD const type = MAKEFOURCC('v', 'i', 'd', 'c');
    DWORD const handler = MAKEFOURCC(handlerFCC_[0], handlerFCC_[1], handlerFCC_[2], handlerFCC_[3]);
    compressor_ = ScopedHIC(ICOpen(type, handler, ICMODE_COMPRESS));
    decompressor_ = ScopedHIC(ICOpen(type, handler, ICMODE_DECOMPRESS));

    while (true) {
        queueMutex_.lock();
        if (queue_.empty()) {
            queueMutex_.unlock();
            if (shouldTerminate_) {
                break;
            }
            std::this_thread::yield();
        } else {
            Size size = queue_.front();
            queue_.pop_front();
            queueMutex_.unlock();

            auto r = findWatermark(size.width_, size.height_);
            if (r) {
                std::lock_guard<std::mutex> lk(resultMutex_);
                results_.push_back(r);
            }
        }
    }
}


bool WatermarkSearchWorker::enqueue(std::vector<Size> const& queue)
{
    std::lock_guard<std::mutex> lk(queueMutex_);
    if (queue.size() + queue_.size() <= kMaxQueueLength) {
        std::copy(queue.begin(), queue.end(), std::back_inserter(queue_));
        return true;
    } else {
        return false;
    }
}


void WatermarkSearchWorker::drainResults(std::vector<std::shared_ptr<WatermarkSearchResult>>& buffer)
{
    buffer.clear();
    std::lock_guard<std::mutex> lk(resultMutex_);
    std::copy(results_.begin(), results_.end(), std::back_inserter(buffer));
    results_.clear();
}


void WatermarkSearchWorker::terminate()
{
    shouldTerminate_ = true;
}


std::shared_ptr<WatermarkSearchResult> WatermarkSearchWorker::findWatermark(int width, int height)
{
    std::shared_ptr<WatermarkSearchResult> r = std::make_shared<WatermarkSearchResult>();
    r->size_.width_ = width;
    r->size_.height_ = height;

    int const bytesPerPixel = 4;
    int const scanlineSizeInBytes = width * bytesPerPixel;

    BITMAPINFOHEADER inputFormat;
    ZeroMemory(&inputFormat, sizeof(inputFormat));
    inputFormat.biSize = sizeof(inputFormat);
    inputFormat.biWidth = width;
    inputFormat.biHeight = height;
    inputFormat.biPlanes = 1;
    inputFormat.biBitCount = 8 * bytesPerPixel;
    inputFormat.biCompression = BI_RGB;
    inputFormat.biSizeImage = scanlineSizeInBytes * height;

    DWORD const size = ICCompressGetFormatSize(compressor_, &inputFormat);
    if (size == 0 || (size & 0xFFFF0000) == 0xFFFF0000) {
        return r;
    }
    std::vector<uint8_t> outputFormatStorage(size);
    LPBITMAPINFOHEADER outputFormat = (LPBITMAPINFOHEADER)(BITMAPINFO*)outputFormatStorage.data();
    if (ICCompressGetFormat(compressor_, &inputFormat, outputFormat) != ICERR_OK) {
        return r;
    }

    DWORD const outputSize = ICCompressGetSize(compressor_, &inputFormat, outputFormat);
    if (outputSize < 0) {
        return r;
    }

    if (outputBuffer_.size() < outputSize) {
        outputBuffer_.resize(outputSize);
    }
    if (inputBuffer_.size() < inputFormat.biSizeImage) {
        inputBuffer_.resize(inputFormat.biSizeImage);
    }
    std::fill(inputBuffer_.begin(), inputBuffer_.begin() + inputFormat.biSizeImage, (uint8_t)0xff);

    if (ICCompressBegin(compressor_, &inputFormat, outputFormat) != ICERR_OK) {
        return r;
    }

    DWORD flags = 0;
    DWORD dwckid = 0;

    DWORD result = ICCompress(compressor_,
                              0, // dwFlags
                              outputFormat,
                              outputBuffer_.data(),
                              &inputFormat,
                              inputBuffer_.data(),
                              &dwckid,
                              &flags,
                              0, // lpFrameNum
                              0, // dwFrameSize
                              0, // dwQuality
                              nullptr, // lpbiPrev
                              nullptr); // lpPrev
    if (result != ICERR_OK) {
        return r;
    }

    if (ICCompressEnd(compressor_) != ICERR_OK) {
        return r;
    }

    if (ICDecompressBegin(decompressor_, outputFormat, &inputFormat) != ICERR_OK) {
        return r;
    }

    std::fill(inputBuffer_.begin(), inputBuffer_.begin() + inputFormat.biSizeImage, (uint8_t)0xff);
    if (ICDecompress(decompressor_, 0, outputFormat, outputBuffer_.data(), &inputFormat, inputBuffer_.data()) != ICERR_OK) {
        return r;
    }

    if (ICDecompressEnd(decompressor_) != ICERR_OK) {
        return r;
    }

    // Search left edge of watermark
    int left = -1;
    for (int x = 0; x < width; ++x) {
        bool found = false;
        for (int y = 0; y < height; ++y) {
            for (int j = 0; j < bytesPerPixel; ++j) {
                int index = scanlineSizeInBytes * y + x * bytesPerPixel + j;
                if (inputBuffer_[index] != (uint8_t)0xff) {
                    found = true;
                    left = x;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found) {
            break;
        }
    }
    if (left < 0) {
        return r;
    }

    // Search right edge of watermark
    int right = -1;
    for (int x = width - 1; x >= 0; --x) {
        bool found = false;
        for (int y = 0; y < height; ++y) {
            for (int j = 0; j < bytesPerPixel; ++j) {
                int index = scanlineSizeInBytes * y + x * bytesPerPixel + j;
                if (inputBuffer_[index] != (uint8_t)0xff) {
                    found = true;
                    right = x;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found) {
            break;
        }
    }
    if (right < 0) {
        return r;
    }

    // Search top edge of watermark
    int top = -1;
    for (int y = 0; y < height; ++y) {
        bool found = false;
        for (int x = 0; x < width; ++x) {
            for (int j = 0; j < bytesPerPixel; ++j) {
                int index = scanlineSizeInBytes * y + x * bytesPerPixel + j;
                if (inputBuffer_[index] != (uint8_t)0xff) {
                    found = true;
                    top = y;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found) {
            break;
        }
    }
    if (top < 0) {
        return r;
    }

    // Search bottom edge of watermark
    int bottom = -1;
    for (int y = height - 1; y >= 0; --y) {
        bool found = false;
        for (int x = 0; x < width; ++x) {
            for (int j = 0; j < bytesPerPixel; ++j) {
                int index = scanlineSizeInBytes * y + x * bytesPerPixel + j;
                if (inputBuffer_[index] != (uint8_t)0xff) {
                    found = true;
                    bottom = y;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found) {
            break;
        }
    }
    if (top < 0) {
        return r;
    }

    r->x_ = left;
    r->y_ = top;

    int const watermarkWidth = right - left + 1;
    int const watermarkHeight = bottom - top + 1;
    r->width_ = watermarkWidth;
    r->height_ = watermarkHeight;

    r->watermark_ = std::make_shared<Bitmap>(watermarkWidth, watermarkHeight);
    for (int y = 0; y < watermarkHeight; ++y) {
        for (int x = 0; x < watermarkWidth; ++x) {
            int index = scanlineSizeInBytes * (y + top) + (x + left) * bytesPerPixel;
            uint8_t red = inputBuffer_[index];
            uint8_t green = inputBuffer_[index + 1];
            uint8_t blue = inputBuffer_[index + 2];
            r->watermark_->setPixel(x, watermarkHeight - y - 1, Color{ red, green, blue });
        }
    }
    return r;
}
