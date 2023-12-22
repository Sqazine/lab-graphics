#pragma once

#include <memory>
#include "labgraphics.h"
class Texture
{
public:
	Texture(Device &device,
			ImageData *texture,
			Format format =Format::R8G8B8A8_UNORM,
			ImageTiling tiling = ImageTiling::OPTIMAL);
	~Texture() = default;

	const GpuImage2D *GetImage() const { return mImage.get(); }
	const ImageView2D *GetImageView() const { return mImage->GetView(); }
	Sampler* GetSampler() const { return mSampler.get(); }

private:
	std::unique_ptr<GpuImage2D> mImage;
	std::unique_ptr<Sampler> mSampler;
};