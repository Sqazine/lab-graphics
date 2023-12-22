#include "ShaderCompiler.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include "VK/Utils.h"
#include "VK/Shader.h"

namespace Parser
{
	struct Shader
	{
		std::string path;
		std::string extension;
		std::vector<std::string> content{};
		std::vector<int> definesTokens{};
	};

	std::string TOKEN_DEFINES = "#DEFINE_BLOCK";

	std::string RAY_HIT_SHADER = "Raytracing";
	std::string RAY_MISS_SHADER = "Raytracing";
	std::string RAY_GEN_SHADER = "Raytracing";
	std::string RAY_SHADOW_SHADER = "Shadow";

	std::map<ShaderDefine, std::string> DEFINES = {
		{ShaderDefine::USE_HDR, "USE_HDR"},
		{ShaderDefine::DEBUG_AO_OUTPUT, "DEBUG_AO_OUTPUT"},
		{ShaderDefine::DEBUG_ALBEDO_OUTPUT, "DEBUG_ALBEDO_OUTPUT"},
		{ShaderDefine::DEBUG_NORMAL_OUTPUT, "DEBUG_NORMAL_OUTPUT"},
	};

	std::vector<std::pair<ShaderStage, Shader>> SHADERS = {
		{ShaderStage::CLOSEST_HIT, {RAY_HIT_SHADER, ".rchit"}},
		{ShaderStage::MISS, {RAY_MISS_SHADER, ".rmiss"}},
		{ShaderStage::RAYGEN, {RAY_GEN_SHADER, ".rgen"}},
		{ShaderStage::MISS, {RAY_SHADOW_SHADER, ".rmiss"}}};
}

ShaderCompiler::ShaderCompiler()
{
	Read();
}

void ShaderCompiler::Read() const
{
	for ( auto &pair : Parser::SHADERS)
	{
		auto shader = pair.second;
		auto name = std::filesystem::path(shader.path + shader.extension).make_preferred();
		std::ifstream inShader(name.string());
		std::string line;
		int i = 0;
		std::vector<std::string> file;
		while (std::getline(inShader, line))
		{
			file.emplace_back(line);

			if (line.find(Parser::TOKEN_DEFINES) != std::string::npos)
				shader.definesTokens.emplace_back(i);

			++i;
		}
		inShader.close();
		shader.content = file;
		pair.second = shader;
	}
}

void ShaderCompiler::GlslangValidator() const
{
	std::vector<std::string> extensionsToCheck = {
		".comp", ".compiled.rchit", ".compiled.rgen", ".compiled.rmiss"};

	std::vector<std::string> extensionsToSkip = {
		".comp.spv", ".compiled.rchit.spv", ".compiled.rgen.spv", ".compiled.rmiss.spv"};

	for (const auto &entry : std::filesystem::directory_iterator("."))
	{
		if (entry.is_directory())
			continue;

		auto path = entry.path();

		bool skip = false;
		for (const auto extension : extensionsToSkip)
			if (path.filename().string().find(extension) != std::string::npos)
			{
				skip = true;
				break;
			}
		if (skip)
			continue;

		for (const auto &extension : extensionsToCheck)
		{
			if (path.filename().string().find(extension) != std::string::npos)
			{
				auto shader = path.filename();
				shader.replace_extension(path.extension().string() + ".spv");
				auto cmd = "glslangValidator --target-env vulkan1.2 -V " + path.string() + " -o " + shader.string();
				std::system(cmd.c_str());
			}
		}
	}
}

void ShaderCompiler::Compile(std::vector<ShaderDefine> defines) const
{
	for (const auto &pair : Parser::SHADERS)
	{
		const auto &shader = pair.second;
		auto name = std::filesystem::path(shader.path + ".compiled" + shader.extension).make_preferred();
		auto path = name.string();
		std::ofstream outShader(path, std::ofstream::trunc);
		int i = 0;
		for (const auto &line : shader.content)
		{
			bool isOriginal = true;

			if (std::find(shader.definesTokens.begin(), shader.definesTokens.end(), i) != shader.definesTokens.end())
			{
				isOriginal = false;
				for (auto define : defines)
					outShader<<"#define " << Parser::DEFINES[define] << std::endl;
			}

			if (isOriginal)
			{
				outShader << line << std::endl;
			}

			++i;
		}
		outShader.close();
	}

	GlslangValidator();
}