/*
 * Copyright (c) Stuart Millman 2023.
 */

#include "gfx.hpp"

#include <iostream>

#include <glfw/glfw3.h>

using namespace sm;

int main()
{
	glfwInit();
	auto* window = glfwCreateWindow(1080, 720, "GFX", nullptr, nullptr);

	gfx::set_error_callback([](const char* msg) {
		GFX_LOG_ERR(msg);
		GFX_ASSERT(false, "");
	});

	gfx::AppInfo appInfo{ "Sandbox App" };
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

	gfx::BufferHandle bufferHandle{};
	gfx::BufferInfo bufferInfo{
		.type = gfx::BufferType::eStorage,
		.size = sizeof(int) * 10,
	};
	if (!gfx::create_buffer(bufferHandle, deviceHandle, bufferInfo))
	{
		throw std::runtime_error("Failed to create GFX buffer!");
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
		gfx::draw(commandListHandle, 3, 1, 0, 0);
		gfx::end(commandListHandle);

		gfx::SubmitInfo submitInfo{
			.commandList = commandListHandle,
			.waitSemaphoreHandle = {}
		};
		gfx::FenceHandle fenceHandle;
		gfx::submit_command_list(submitInfo, &fenceHandle, nullptr);
		gfx::wait_on_fence(fenceHandle);
	}

	gfx::destroy_buffer(bufferHandle);

	gfx::destroy_device(deviceHandle);
	gfx::shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();
}