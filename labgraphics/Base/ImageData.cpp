#include "ImageData.h"

ImageData::ImageData() : type(ImageDataType::RGBA8), texWidth(32), texHeight(32), texChannels(4), imageSize(texHeight * texWidth * texChannels)
{
	pixels = new unsigned char[texHeight * texWidth * texChannels];
}

ImageData::ImageData(ImageDataType type, int width, int height, int channel, void *pixels)
	: type(type), pixels(pixels), texWidth(width), texHeight(height),
	  texChannels(channel), imageSize(texHeight * texWidth * texChannels) {}

ImageData::ImageData(ImageData &&texture)
{
	type = texture.type;
	texHeight = texture.texHeight;
	texWidth = texture.texWidth;
	texChannels = texture.texChannels;
	imageSize = texture.imageSize;
	pixels = texture.pixels;

	texture.pixels = nullptr;
}

ImageData &ImageData::operator=(ImageData &&texture)
{
	if (this != &texture)
	{
		type = texture.type;
		texHeight = texture.texHeight;
		texWidth = texture.texWidth;
		texChannels = texture.texChannels;
		imageSize = texture.imageSize;
		pixels = texture.pixels;

		texture.pixels = nullptr;
	}

	return *this;
}

bool ImageData::operator==(ImageData *other)
{
	return type == other->type &&
		   texWidth == other->texWidth &&
		   texHeight == other->texHeight &&
		   texChannels == other->texChannels &&
		   imageSize == other->imageSize &&
		   pixels == other->pixels;
}

ImageData::~ImageData()
{
	if (pixels != nullptr && type != ImageDataType::HDR)
	{
		delete pixels;
		pixels = nullptr;
	}
}