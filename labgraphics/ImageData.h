#pragma once

#include <future>
#include <string>

enum class ImageDataType
{
	R8,
	RG8,
	RGB8,
	RGBA8,
	R16,
	RG16,
	RGB16,
	RGBA16,
	HDR
};

class ImageData
{
public:
	ImageData();
	ImageData(ImageDataType type, int width, int height, int channel, void *pixels);

	ImageData(ImageData &&);

	ImageData &operator=(ImageData &&);

	bool operator==(ImageData *other);
	~ImageData();

	int GetWidth() const { return texWidth; }
	int GetHeight() const { return texHeight; }
	int GetImageSize() const { return imageSize; }
	int GetChannels() const { return mChannels; }

	template <typename T>
	T *GetPixels() const
	{
		return reinterpret_cast<T *>(mPixels);
	}

	int32_t BytesPerPixel() const
	{
		return mChannels * (mHdr ? sizeof(float) : sizeof(uint8_t));
	}

private:
	ImageDataType type;
	void *mPixels;
	int texWidth;
	int texHeight;
	int mChannels;
	int imageSize;
	bool mHdr;
};