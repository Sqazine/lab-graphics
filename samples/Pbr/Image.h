#pragma once
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>
#include <cstdint>
#include <exception>
#include <stdexcept>
class Image
{
public:
    Image(const std::string &filePath, int32_t channels = 4);
    int32_t BytesPerPixel() const;
    int32_t Pitch();

    template <typename T>
    const T *pixels() const;

    int32_t mWidth;
    int32_t mHeight;
    int32_t mChannels;

    bool mHdr;

private:
    std::unique_ptr<uint8_t> mPixels;
};

template <typename T>
inline const T *Image::pixels() const
{
    return reinterpret_cast<const T *>(mPixels.get());
}