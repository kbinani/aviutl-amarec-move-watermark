#pragma once

#include "ScopedHIC.hpp"
#include "Size.hpp"

class WatermarkSearchResult;

class WatermarkSearchWorker
{
public:
    WatermarkSearchWorker(std::string const& handlerFCC);

    void run();

    bool enqueue(std::vector<Size> const& queue);

    void drainResults(std::vector<std::shared_ptr<WatermarkSearchResult>>& buffer);

    void terminate();

    std::shared_ptr<WatermarkSearchResult> findWatermark(size_t width, size_t height);

public:
    static size_t const kMaxQueueLength = 64;

private:
    ScopedHIC compressor_;
    ScopedHIC decompressor_;
    std::vector<std::shared_ptr<WatermarkSearchResult>> results_;
    std::vector<uint8_t> outputBuffer_;
    std::vector<uint8_t> inputBuffer_;
    std::deque<Size> queue_;
    std::mutex queueMutex_;
    std::atomic<bool> shouldTerminate_{ false };
    std::mutex resultMutex_;
    std::string handlerFCC_;
};
