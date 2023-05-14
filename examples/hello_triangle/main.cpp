/*
 * Copyright (c) Stuart Millman 2023.
 */

#include "gfx.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if _WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#else
	#error "Unsupported platform!"
#endif
#include <GLFW/glfw3native.h>

#include <iostream>
#include <fstream>

using namespace sm;

auto read_shader_file(const char* filename) -> std::vector<char>
{
	if (std::ifstream file{ filename, std::ios::binary | std::ios::ate })
	{
		const std::streamsize fileSize = file.tellg();
		file.seekg(0);
		std::vector<char> shaderBinary(fileSize);
		file.read(shaderBinary.data(), fileSize);
		return shaderBinary;
	}

	GFX_LOG_ERR_FMT("Example - compute - Failed to read shader file: {}", filename);
	return {};
}

int main()
{
	constexpr std::uint32_t WINDOW_WIDTH = 1080;
	constexpr std::uint32_t WINDOW_HEIGHT = 720;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello Triangle", nullptr, nullptr);

	gfx::set_error_callback([](const char* msg) {
		GFX_LOG_ERR(msg);
		GFX_ASSERT(false, "");
	});

	gfx::AppInfo appInfo{ "Hello Triangle App" };
	if (!gfx::initialise(appInfo))
	{
		throw std::runtime_error("Failed to initialise GFX!");
	}

	gfx::DeviceInfo device_info{
		.deviceFlags = gfx::DeviceFlags_PreferDiscrete,
		.queueFlags = { gfx::QueueFlags_Graphics },
	};
	gfx::DeviceHandle deviceHandle{};
	if (!gfx::create_device(deviceHandle, device_info))
	{
		throw std::runtime_error("Failed to create GFX device!");
	}

	void* platformWindowHandle{ nullptr };
#if _WIN32
	platformWindowHandle = glfwGetWin32Window(window);
#endif

	gfx::SwapChainInfo swapChainInfo{
		.platformWindowHandle = platformWindowHandle,
		.initialWidth = WINDOW_WIDTH,
		.initialHeight = WINDOW_HEIGHT,
	};
	gfx::SwapChainHandle swapChainHandle{};
	if (!gfx::create_swap_chain(swapChainHandle, deviceHandle, swapChainInfo))
	{
		throw std::runtime_error("Failed to create GFX swap chain!");
	}

	const auto vertShaderBinary = read_shader_file("triangle.vert.spv");
	const auto fragShaderBinary = read_shader_file("triangle.frag.spv");
	gfx::GraphicsPipelineInfo pipelineInfo{
		.vertexCode = vertShaderBinary,
		.fragmentCode = fragShaderBinary,
		.descriptorSets = {},
	};
	gfx::PipelineHandle pipelineHandle{};
	if (!gfx::create_graphics_pipeline(pipelineHandle, deviceHandle, pipelineInfo))
	{
		throw std::runtime_error("Failed to create GFX graphics pipeline!");
	}

	gfx::CommandListHandle commandListHandle{};
	if (!gfx::create_command_list(commandListHandle, deviceHandle, 0))
	{
		throw std::runtime_error("Failed to create GFX command list!");
	}

	while (glfwWindowShouldClose(window) == 0)
	{
		glfwPollEvents();

		gfx::reset(commandListHandle);
		gfx::begin(commandListHandle);
		// #TODO: Begin Render Pass
		// #TODO: Set viewport
		// #TODO: Set scissor
		gfx::bind_pipeline(commandListHandle, pipelineHandle);
		// #TODO: Draw
		// #TODO: End Render Pass
		gfx::end(commandListHandle);

		gfx::SubmitInfo submitInfo{
			.commandList = commandListHandle,
			.waitSemaphoreHandle = {}
		};
		gfx::FenceHandle fenceHandle;
		gfx::submit_command_list(submitInfo, &fenceHandle, nullptr);

		gfx::present_swap_chain(swapChainHandle, 0, nullptr);

		gfx::wait_on_fence(fenceHandle);
	}

	gfx::destroy_swap_chain(swapChainHandle);

	gfx::destroy_device(deviceHandle);
	gfx::shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();
}