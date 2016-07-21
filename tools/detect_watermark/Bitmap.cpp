#include "Bitmap.hpp"

class Bitmap::Impl
{
public:
    Impl(size_t width, size_t height)
    {
        bitmap_ = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat24bppRGB);
    }

    void save(std::string const& file, std::wstring const& mimeType) const
    {
        std::tr2::sys::path filePath(file);
        auto classId = findClassId(mimeType);
        bitmap_->Save(filePath.c_str(), classId.get());
    }

    std::string md5() const
    {
        Gdiplus::Rect rect;
        rect.X = 0;
        rect.Y = 0;
        rect.Width = bitmap_->GetWidth();
        rect.Height = bitmap_->GetHeight();
        auto data = std::make_unique<Gdiplus::BitmapData>();
        bitmap_->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, data.get());
        auto md5 = md5sum(data->Scan0, data->Height * data->Stride);
        bitmap_->UnlockBits(data.get());
        return md5;
    }

    void setPixel(size_t x, size_t y, Color color)
    {
        bitmap_->SetPixel(x, y, Gdiplus::Color(color.red_, color.green_, color.blue_));
    }

    Color pixel(size_t x, size_t y) const
    {
        Gdiplus::Color color;
        bitmap_->GetPixel(x, y, &color);
        Color c;
        c.red_ = color.GetRed();
        c.green_ = color.GetGreen();
        c.blue_ = color.GetBlue();
        return c;
    }

private:
    std::shared_ptr<CLSID> findClassId(std::wstring const& mimeType) const
    {
        std::lock_guard<std::mutex> lk(classIdMutex_);

        auto it = classId_.find(mimeType);
        if (it == classId_.end()) {
            auto classId = getEncoderClassId(mimeType);
            classId_[mimeType] = classId;
            return classId;
        } else {
            return it->second;
        }
    }

    static std::shared_ptr<CLSID> getEncoderClassId(std::wstring const& inMimeType)
    {
        UINT num = 0, size = 0;
        Gdiplus::GetImageEncodersSize(&num, &size);
        if (size == 0) {
            return nullptr;
        }

        std::vector<uint8_t> storage(size);
        Gdiplus::ImageCodecInfo* info = reinterpret_cast<Gdiplus::ImageCodecInfo*>(storage.data());

        Gdiplus::GetImageEncoders(num, size, info);

        for (UINT i = 0; i < num; ++i) {
            std::wstring const mimeType = info[i].MimeType;
            if (mimeType == inMimeType) {
                return std::make_shared<CLSID>(info[i].Clsid);
            }
        }

        return nullptr;
    }

    static std::string md5sum(void const* data, size_t data_size)
    {
        HCRYPTPROV hProv = NULL;

        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            return "";
        }

        HCRYPTPROV hHash = NULL;
        BOOL hash_ok = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);

        if (!hash_ok) {
            CryptReleaseContext(hProv, 0);
            return "";
        }

        if (!CryptHashData(hHash, reinterpret_cast<BYTE const*>(data), data_size, 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        DWORD cbHashSize = 0, dwCount = sizeof(DWORD);
        if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&cbHashSize, &dwCount, 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        std::vector<BYTE> buffer(cbHashSize);
        if (!CryptGetHashParam(hHash, HP_HASHVAL, reinterpret_cast<BYTE *>(&buffer[0]), &cbHashSize, 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        std::ostringstream oss;
        std::for_each(buffer.begin(), buffer.end(), [&oss](BYTE b) {
            oss.fill('0');
            oss.width(2);
            oss << std::hex << static_cast<int>(b);
        });

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return oss.str();
    }

private:
    std::unique_ptr<Gdiplus::Bitmap> bitmap_;

    static std::mutex classIdMutex_;
    static std::map<std::wstring, std::shared_ptr<CLSID>> classId_;
};


std::mutex Bitmap::Impl::classIdMutex_;
std::map<std::wstring, std::shared_ptr<CLSID>> Bitmap::Impl::classId_;


Bitmap::Bitmap(size_t width, size_t height)
    : impl_(std::make_unique<Impl>(width, height))
{
}


Bitmap::~Bitmap()
{}


void Bitmap::save(std::string const& file, std::wstring const& mimeType) const
{
    impl_->save(file, mimeType);
}


std::string Bitmap::md5() const
{
    return impl_->md5();
}


void Bitmap::setPixel(size_t x, size_t y, Color color)
{
    impl_->setPixel(x, y, color);
}


Color Bitmap::pixel(size_t x, size_t y) const
{
    return impl_->pixel(x, y);
}
