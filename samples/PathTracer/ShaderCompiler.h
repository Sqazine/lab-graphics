#pragma once

#include <map>
#include <vector>
#include <algorithm>
#include "VK/Utils.h"

enum class ShaderDefine
{
	USE_HDR,
	DEBUG_AO_OUTPUT,
	DEBUG_ALBEDO_OUTPUT,
	DEBUG_NORMAL_OUTPUT,
};

class ShaderCompiler
{
public:
	ShaderCompiler();
	~ShaderCompiler() = default;
	void Compile(std::vector<ShaderDefine> defines) const;

private:
	void Read() const;
	void GlslangValidator() const;
};
