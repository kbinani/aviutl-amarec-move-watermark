#pragma once

class ScopedHIC
{
public:
    ScopedHIC(HIC handle)
        : handle_(handle)
    {
    }

    ~ScopedHIC()
    {
        if (handle_) {
            ICClose(handle_);
            handle_ = nullptr;
        }
    }

    ScopedHIC(ScopedHIC const& other) = delete;
    ScopedHIC& operator = (ScopedHIC const& other) = delete;

    ScopedHIC(ScopedHIC&& rhs)
    {
        assert(handle_ == nullptr);
        handle_ = rhs.handle_;
        rhs.handle_ = nullptr;
    }

    ScopedHIC& operator = (ScopedHIC&& rhs)
    {
        assert(handle_ == nullptr);
        handle_ = rhs.handle_;
        rhs.handle_ = nullptr;
        return *this;
    }

    operator HIC() const
    {
        return handle_;
    }

private:
    HIC handle_ = nullptr;
};
