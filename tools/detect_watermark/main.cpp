#include "OptionParser.h"
#include "ScopedGdiplusInitializer.hpp"
#include "SizeComparator.hpp"
#include "WatermarkSearchResult.hpp"
#include "ScopedHIC.hpp"
#include "Bitmap.hpp"
#include "WatermarkSearchWorker.hpp"
#include "ExistingSearchResult.hpp"

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "vfw32.lib")

DWORD makeFCC(std::string const& fcc)
{
    assert(fcc.size() == 4);
    return MAKEFOURCC(fcc[0], fcc[1], fcc[2], fcc[3]);
}

int main(int argc, char *argv[])
{
    optparse::OptionParser parser;
    parser.add_option("--min-width").dest("min_width").help("minimum image width to test").metavar("MIN_WIDTH").set_default(1);
    parser.add_option("--max-width").dest("max_width").help("maximum image width to test").metavar("MAX_WIDTH").set_default(std::numeric_limits<uint16_t>::max());
    parser.add_option("--min-height").dest("min_height").help("minimum image height to test").metavar("MIN_HEIGHT").set_default(1);
    parser.add_option("--max-height").dest("max_height").help("maximum image height to test").metavar("MAX_HEIGHT").set_default(std::numeric_limits<uint16_t>::max());
    parser.add_option("--codec-fcc").dest("handler_fcc").help("Codec's FCC (four character code)").metavar("FCC").set_default("amv4");
    parser.add_option("--width-step").dest("width_step").set_default(1);
    parser.add_option("--height-step").dest("height_step").set_default(1);
    parser.add_option("--concurrency").dest("concurrency").set_default(std::thread::hardware_concurrency());
    auto options = parser.parse_args(argc, argv);

    std::string const handlerFCC = options.get("handler_fcc");
    if (handlerFCC.size() != 4) {
        parser.error("length of FCC should be 4");
    }
    {
        ScopedHIC compressor = ICOpen(makeFCC("vidc"), makeFCC(handlerFCC), ICMODE_COMPRESS);
        if ((HIC)compressor == nullptr) {
            parser.error("invalid fcc");
        }
    }
    {
        ScopedHIC decompressor = ICOpen(makeFCC("vidc"), makeFCC(handlerFCC), ICMODE_DECOMPRESS);
        if ((HIC)decompressor == nullptr) {
            parser.error("invalid fcc");
        }
    }

    int const minWidth = std::stoi((char const*)options.get("min_width"));
    int const maxWidth = std::stoi((char const*)options.get("max_width"));
    if (maxWidth < minWidth) {
        parser.error("minWidth and maxWidth should be: minWidth <= maxWidth");
    }
    int const minHeight = std::stoi((char const*)options.get("min_height"));
    int const maxHeight = std::stoi((char const*)options.get("max_height"));
    if (maxHeight < minHeight) {
        parser.error("minHeight and maxHeight should be: minHeight <= maxHeight");
    }

    int const widthStep = std::stoi((char const*)options.get("width_step"));
    if (widthStep <= 0) {
        parser.error("widthStep should be grater than 0");
    }
    int const heightStep = std::stoi((char const*)options.get("height_step"));
    if (heightStep <= 0) {
        parser.error("heightStep should be grater than 0");
    }

    ScopedGdiplusInitializer gdiInitializer;

    ExistingSearchResult existing(minWidth, maxWidth, widthStep, minHeight, maxHeight, heightStep);
    std::string const logFileName = handlerFCC + std::string(".log");
    bool const logFileExists = std::tr2::sys::exists(logFileName);
    if (logFileExists) {
        existing.load(logFileName);
    }

    std::ofstream log(logFileName, std::ios_base::app);
    if (!logFileExists) {
        log << "#width\theight\twatermark_left\twwatermark_bottom\twatermark_width\twatermark_height" << std::endl;
    }

    unsigned const numWorkers = std::stoi((char const*)options.get("concurrency"));

    std::vector<std::unique_ptr<WatermarkSearchWorker>> workers;
    std::vector<std::thread> threads;

    for (unsigned i = 0; i < numWorkers; ++i) {
        workers.emplace_back(new WatermarkSearchWorker(handlerFCC));
    }

    if (numWorkers > 1) {
        for (unsigned i = 0; i < numWorkers; ++i) {
            threads.emplace_back([](WatermarkSearchWorker* worker) {
                worker->run();
            }, workers[i].get());
        }
    }

    std::vector<Size> queueBuffer;
    int const enqueUnitSize = WatermarkSearchWorker::kMaxQueueLength / 2;

    std::vector<std::shared_ptr<WatermarkSearchResult>> results;
    int index = 0;

    auto enqueue = [=, &queueBuffer, &index, &workers, &log, &results](Size size) {
        if (numWorkers > 1) {
            queueBuffer.push_back(size);
            if (queueBuffer.size() >= enqueUnitSize) {
                while (true) {
                    bool const queued = workers[index]->enqueue(queueBuffer);
                    index = (index + 1) % numWorkers;
                    if (queued) {
                        break;
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                queueBuffer.clear();
            }

            for (unsigned i = 0; i < numWorkers; ++i) {
                workers[i]->drainResults(results);
                for (auto const& result : results) {
                    result->print(handlerFCC, log);
                }
            }
        } else {
            auto result = workers[0]->findWatermark(size.width_, size.height_);
            result->print(handlerFCC, log);
        }
    };

    int const imin = std::min(minWidth, minHeight);
    int const imax = std::max(maxWidth, maxHeight);
    for (int i = imin; i <= imax; ++i) {
        std::cout << "\r" << i;
        {
            int h = i;
            if ((h - minHeight) % heightStep == 0) {
                for (int w = minWidth; w <= maxWidth; w += widthStep) {
                    if (w == h) {
                        continue;
                    }
                    if (w > i) {
                        break;
                    }
                    Size size;
                    size.width_ = w;
                    size.height_ = h;
                    if (existing.queryAlreadyTested(size)) {
                        continue;
                    }
                    enqueue(size);
                }
            }
        }
        {
            int w = i;
            if ((w - minWidth) % widthStep == 0) {
                for (int h = minHeight; h <= maxHeight; h += heightStep) {
                    if (h > i) {
                        break;
                    }
                    Size size;
                    size.width_ = w;
                    size.height_ = h;
                    if (existing.queryAlreadyTested(size)) {
                        continue;
                    }
                    enqueue(size);
                }
            }
        }
    }

    if (!queueBuffer.empty()) {
        bool ok = false;
        while (true) {
            for (unsigned i = 0; i < numWorkers; ++i) {
                if (workers[i]->enqueue(queueBuffer)) {
                    ok = true;
                    queueBuffer.clear();
                    break;
                }
            }
            if (ok) {
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    if (numWorkers > 1) {
        for (unsigned i = 0; i < numWorkers; ++i) {
            workers[i]->terminate();
        }
        for (unsigned i = 0; i < numWorkers; ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }

        for (unsigned i = 0; i < numWorkers; ++i) {
            workers[i]->drainResults(results);
            for (auto const& result : results) {
                result->print(handlerFCC, log);
            }
        }
    }

    return 0;
}
