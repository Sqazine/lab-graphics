#include "ImageData.h"

ImageData::ImageData() : type(ImageDataType::RGBA8), texWidth(32), texHeight(32), mChannels(4), imageSize(texHeight * texWidth * mChannels)
{
	mPixels = new uint8_t[texHeight * texWidth * mChannels];
}

ImageData::ImageData(ImageDataType type, int width, int height, int channel, void *pixels)
	: type(type), mPixels(pixels), texWidth(width), texHeight(height),
	  mChannels(channel), imageSize(texHeight * texWidth * mChannels)
{
}

ImageData::ImageData(ImageData &&texture)
{
	type = texture.type;
	texHeight = texture.texHeight;
	texWidth = texture.texWidth;
	mChannels = texture.mChannels;
	imageSize = texture.imageSize;
	mPixels = texture.mPixels;

	texture.mPixels = nullptr;
}

ImageData &ImageData::operator=(ImageData &&texture)
{
	if (this != &texture)
	{
		type = texture.type;
		texHeight = texture.texHeight;
		texWidth = texture.texWidth;
		mChannels = texture.mChannels;
		imageSize = texture.imageSize;
		mPixels = texture.mPixels;

		texture.mPixels = nullptr;
	}

	return *this;
}

bool ImageData::operator==(ImageData *other)
{
	return type == other->type &&
		   texWidth == other->texWidth &&
		   texHeight == other->texHeight &&
		   mChannels == other->mChannels &&
		   imageSize == other->imageSize &&
		   mPixels == other->mPixels;
}

ImageData::~ImageData()
{
	if (mPixels != nullptr && type != ImageDataType::HDR)
	{
		delete mPixels;
		mPixels = nullptr;
	}
}