#pragma once

class ScopedGdiplusInitializer
{
public:
    ScopedGdiplusInitializer()
    {
        Gdiplus::GdiplusStartup(&token_, &startupInput_, nullptr);
    }

    ~ScopedGdiplusInitializer()
    {
        Gdiplus::GdiplusShutdown(token_);
    }

private:
    Gdiplus::GdiplusStartupInput startupInput_;
    ULONG_PTR token_;
};
