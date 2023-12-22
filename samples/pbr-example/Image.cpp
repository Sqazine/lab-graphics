#include "Image.h"
#include <stb/stb_image.h>
#include "Logger.h"
Image::Image(const std::string &filePath, int32_t channels)
{
    LOG_INFO("Loading image: {}\n", filePath);

    if (stbi_is_hdr(filePath.c_str()))
    {
        float *pixels = stbi_loadf(filePath.c_str(), &mWidth, &mHeight, &mChannels, channels);
        if (pixels)
        {
            mPixels.reset(reinterpret_cast<unsigned char *>(pixels));
            mHdr = true;
        }
    }
    else
    {
        unsigned char *pixels = stbi_load(filePath.c_str(), &mWidth, &mHeight, &mChannels, channels);
        if (pixels)
        {
            mPixels.reset(pixels);
            mHdr = false;
        }
    }
    if (channels > 0)
        mChannels = channels;

    if (!mPixels)
    {
        throw std::runtime_error("Failed to load image file: " + filePath);
    }
}

int32_t Image::BytesPerPixel() const
{
    return mChannels * (mHdr ? sizeof(float) : sizeof(uint8_t));
}

int32_t Image::Pitch()
{
    return mWidth * BytesPerPixel();
}