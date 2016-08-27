#include "amarec_move_watermark.h"
#include "Mask.h"
#include "MaskPixel.h"
#include "Colorspace.h"

EXTERN_C FILTER_DLL __declspec(dllexport)* __stdcall GetFilterTable()
{
    static bool s_initialized = false;
    static FILTER_DLL s_filter;
    static char* track_name[] = { "delta X", "delta Y" };
    static int track_default[] = { 0, 0 };
    static int track_s[] = { 0, 0 };
    static int track_e[] = { 100, 100 };

    if (!s_initialized) {
        memset(&s_filter, 0, sizeof(s_filter));

        s_filter.flag            = FILTER_FLAG_EX_INFORMATION;
        s_filter.x               = 0;
        s_filter.y               = 0;
        s_filter.name            = "amarec_move_watermark";
        s_filter.track_n         = 2;
        s_filter.track_name      = track_name;
        s_filter.track_default   = track_default;
        s_filter.track_s         = track_s;
        s_filter.track_e         = track_e;
        s_filter.check_n         = 0;
        s_filter.check_name      = nullptr;
        s_filter.check_default   = nullptr;
        s_filter.func_proc       = func_proc;
        s_filter.func_WndProc    = func_WndProc;
        s_filter.information     = "amarec_move_watermark";

        s_initialized = true;
    }
    return &s_filter;
}

static int s_filtered_width = 0;
static int s_filtered_height = 0;

BOOL func_proc(FILTER* fp, FILTER_PROC_INFO *fpip)
{
    if (!fp) {
        return FALSE;
    }
    if (!fpip) {
        return FALSE;
    }
    if (!fpip->ycp_edit) {
        return FALSE;
    }
    int const width = fpip->w;
    int const height = fpip->h;

    s_filtered_width = width;
    s_filtered_height = height;

    int bottomMargin, leftMargin;
    Mask const* mask = Mask::selectMask(width, height, leftMargin, bottomMargin);
    if (mask == nullptr) {
        return TRUE;
    }

    int const xoffset = leftMargin;
    int const yoffset = height - bottomMargin - mask->height_;

    int minMaskY = std::max(0, -yoffset);
    int maxMaskY = std::min(mask->height_ - 1, height - 1 - yoffset);

    int minMaskX = std::max(0, -xoffset);
    int maxMaskX = std::min(mask->width_ - 1, width - 1 - xoffset);

    for (int maskY = minMaskY; maskY <= maxMaskY; ++maskY) {
        int const y = yoffset + maskY;
        assert(0 <= y && y < height);

        PIXEL_YC* pixel = fpip->ycp_edit + y * fpip->max_w + minMaskX + xoffset;
        MaskPixel const* maskPtr = mask->pixelData_ + maskY * mask->width_ + minMaskX;

        for (int maskX = minMaskX; maskX <= maxMaskX; ++maskX) {
            float r, g, b;
            Colorspace::getRGBFromYCbCr(pixel->y, pixel->cb, pixel->cr, r, g, b);
            float recoveredR, recoveredG, recoveredB;
            mask->unmask(maskPtr, r, g, b, recoveredR, recoveredG, recoveredB);
            Colorspace::getYCbCrFromRGB(recoveredR, recoveredG, recoveredB, pixel->y, pixel->cb, pixel->cr);
            ++pixel;
            ++maskPtr;
        }
    }

    int const xShift = fp->track[0];
    int const yShift = fp->track[1];

    minMaskY = std::max(0, -yoffset - yShift);
    maxMaskY = std::min(mask->height_ - 1, height - 1 - yoffset - yShift);

    minMaskX = std::max(0, -xoffset - xShift);
    maxMaskX = std::min(mask->width_ - 1, width - 1 - xoffset - xShift);

    for (int maskY = minMaskY; maskY <= maxMaskY; ++maskY) {
        int const y = yoffset + maskY + yShift;
        assert(0 <= y && y < height);

        PIXEL_YC* pixel = fpip->ycp_edit + y * fpip->max_w + xoffset + minMaskX + xShift;
        MaskPixel const* maskPtr = mask->pixelData_ + maskY * mask->width_ + minMaskX;

        for (int maskX = minMaskX; maskX <= maxMaskX; ++maskX) {
            float r, g, b;
            Colorspace::getRGBFromYCbCr(pixel->y, pixel->cb, pixel->cr, r, g, b);
            float recoveredR, recoveredG, recoveredB;
            mask->mask(maskPtr, r, g, b, recoveredR, recoveredG, recoveredB);
            Colorspace::getYCbCrFromRGB(recoveredR, recoveredG, recoveredB, pixel->y, pixel->cb, pixel->cr);
            ++pixel;
            ++maskPtr;
        }
    }

    return TRUE;
}


BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
{
    if (!fp) {
        return FALSE;
    }
    if (!editp) {
        return FALSE;
    }

    if (fp->exfunc->is_saving(editp)) {
        return FALSE;
    }

    if (s_filtered_width <= 0 || s_filtered_height <= 0) {
        return FALSE;
    }

    int leftMargin, bottomMargin;
    Mask const* mask = Mask::selectMask(s_filtered_width, s_filtered_height, leftMargin, bottomMargin);
    if (!mask) {
        return FALSE;
    }

    int const minXShift = -leftMargin;
    int const maxXShift = s_filtered_width - mask->width_ - leftMargin;

    int const minYShift = -(s_filtered_height - bottomMargin - mask->height_);
    int const maxYShift = bottomMargin;

    bool changed = false;
    changed |= fp->track_s[0] != minXShift;
    changed |= fp->track_e[0] != maxXShift;
    changed |= fp->track_s[1] != minYShift;
    changed |= fp->track_e[1] != maxYShift;

    if (changed) {
        fp->track_s[0] = minXShift;
        fp->track_e[0] = maxXShift;

        fp->track_s[1] = minYShift;
        fp->track_e[1] = maxYShift;

        char const* kClassName = "msctls_trackbar32";
        HWND deltaX = FindWindowExA(hwnd, nullptr, kClassName, nullptr);
        if (deltaX) {
            SendMessageA(deltaX, TBM_SETRANGE, (WPARAM)FALSE, (LPARAM)MAKELONG(minXShift, maxXShift));
            HWND deltaY = FindWindowExA(hwnd, deltaX, kClassName, nullptr);
            if (deltaY) {
                SendMessageA(deltaY, TBM_SETRANGE, (WPARAM)FALSE, (LPARAM)MAKELONG(minYShift, maxYShift));
            }
        }
    }

    return FALSE;
}
