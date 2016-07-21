#include "Mask.h"
#include "Size.hpp"
#include "SizeComparator.hpp"
#include "ExistingSearchResult.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        return -1;
    }

    std::string const file = argv[1];

    std::ofstream result("validate_mask.log", std::ios::app);

    std::ifstream stream(file);
    std::vector<std::string> tokens;
    for (std::string line; std::getline(stream, line); ) {
        if (line.empty()) {
            continue;
        }
        if (line[0] == '#') {
            continue;
        }

        ExistingSearchResult::split(line, '\t', tokens);
        if (tokens.size() < 2) {
            continue;
        }

        int const width = std::stoi(tokens[0]);
        int const height = std::stoi(tokens[1]);
        int left, bottom;
        Mask const* mask = Mask::selectMask(width, height, left, bottom);

        if (tokens.size() == 2) {
            if (mask != nullptr) {
                result << width << "\t" << height << "\t" << "mask should be NULL" << std::endl;
            }
        } else if (tokens.size() == 6) {
            int expectedLeft = std::stoi(tokens[2]);
            int expectedBottom = std::stoi(tokens[3]);
            int w = std::stoi(tokens[4]);
            int h = std::stoi(tokens[5]);
            if (mask == nullptr) {
                result << width << "\t" << height << "\t" << "mask hould not be NULL" << std::endl;
            } else {
                if (mask->width_ != w) {
                    result << width << "\t" << height << "\t" << "mask->width_ " << w << " expected for " << mask->width_ << std::endl;
                }
                if (mask->height_ != h) {
                    result << width << "\t" << height << "\t" << "mask->height_ " << h << " expected for " << mask->height_ << std::endl;
                }
                if (left != expectedLeft) {
                    result << width << "\t" << height << "\t" << "left " << expectedLeft << " expected for " << left << std::endl;
                }
                if (bottom != expectedBottom) {
                    result << width << "\t" << height << "\t" << "bottom " << expectedBottom << " expected for " << bottom << std::endl;
                }
            }
        } else {
            assert(false);
        }
    }

    return 0;
}
