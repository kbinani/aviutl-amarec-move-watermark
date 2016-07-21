#pragma once

#include "Size.hpp"
#include "SizeComparator.hpp"

class ExistingSearchResult
{
public:
    ExistingSearchResult(int minWidth, int maxWidth, int widthStep, int minHeight, int maxHeight, int heightStep)
        : minWidth_(minWidth)
        , maxWidth_(maxWidth)
        , widthStep_(widthStep)
        , minHeight_(minHeight)
        , maxHeight_(maxHeight)
        , heightStep_(heightStep)
    {
        minCompletedWidth_ = minWidth_ - widthStep_;
        minCompletedHeight_ = minHeight_ - heightStep_;
    }

    void load(std::string const& file)
    {
        existing_.clear();

        std::ifstream stream(file);
        std::vector<std::string> tokens;
        for (std::string line; std::getline(stream, line); ) {
            if (line.empty()) {
                continue;
            }
            if (line[0] == '#') {
                continue;
            }
            split(line, '\t', tokens);
            if (tokens.size() < 2) {
                continue;
            }
            Size size;
            size.width_ = std::stoi(tokens[0]);
            size.height_ = std::stoi(tokens[1]);

            if (minWidth_ <= size.width_ && size.width_ <= maxWidth_ &&
                minHeight_ <= size.height_ && size.height_ <= maxHeight_) {
                existing_.insert(size);
            }
        }

        bool changed = true;
        while (changed) {
            changed = false;

            // increment minCompletedWidth_ then test
            bool ok = true;
            for (int height = minHeight_; height <= minCompletedHeight_; height += heightStep_) {
                Size size;
                size.width_ = minCompletedWidth_ + widthStep_;
                size.height_ = height;
                auto it = existing_.find(size);
                if (it == existing_.end()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                changed = true;
                minCompletedWidth_ += widthStep_;
            }

            // increment minCompletedHeight_ then test
            ok = true;
            for (int width = minWidth_; width <= minCompletedWidth_; width += widthStep_) {
                Size size;
                size.width_ = width;
                size.height_ = minCompletedHeight_ + heightStep_;
                auto it = existing_.find(size);
                if (it == existing_.end()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                changed = true;
                minCompletedHeight_ += heightStep_;
            }
        }

        for (int width = minWidth_; width <= minCompletedWidth_; width += widthStep_) {
            for (int height = minHeight_; height <= minCompletedHeight_; height += heightStep_) {
                Size size;
                size.width_ = width;
                size.height_ = height;
                existing_.erase(size);
            }
        }
    }

    bool queryAlreadyTested(Size const& size)
    {
        if (minWidth_ <= size.width_ && size.width_ <= minCompletedWidth_ &&
            minHeight_ <= size.height_ && size.height_ <= minCompletedHeight_) {
            return true;
        }
        auto it = existing_.find(size);
        bool const alreadyTested = it != existing_.end();
        if (alreadyTested) {
            existing_.erase(it);
        }
        return alreadyTested;
    }

    static void split(std::string const& str, char delimiter, std::vector<std::string>& tokens)
    {
        tokens.clear();

        size_t offset = 0;
        while (true) {
            size_t index = str.find(delimiter, offset);
            if (index == std::string::npos) {
                if (offset < str.size()) {
                    std::string token = str.substr(offset);
                    if (!token.empty()) {
                        tokens.push_back(token);
                    }
                }
                break;
            } else {
                std::string token = str.substr(offset, index - offset);
                if (!token.empty()) {
                    tokens.push_back(token);
                }
                offset = index + 1;
            }
        }
    }

private:
    int const minWidth_;
    int const maxWidth_;
    int const widthStep_;
    int const minHeight_;
    int const maxHeight_;
    int const heightStep_;
    std::set<Size, SizeComparator> existing_;

    int minCompletedWidth_;
    int minCompletedHeight_;
};
