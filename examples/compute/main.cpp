/*
 * Copyright (c) Stuart Millman 2023.
 */

#include "gfx/gfx.hpp"

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
	gfx::set_error_callback([](const char* msg) {
		GFX_LOG_ERR(msg);
		GFX_ASSERT(false, "");
	});

	gfx::AppInfo appInfo{ "compute App" };
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

	const auto shaderBinary = read_shader_file("compute.spv");
	gfx::ComputePipelineInfo pipelineInfo{
		.shaderCode = shaderBinary,
		.descriptorSets = {
			gfx::DescriptorSetInfo{ .bindings = {
										{ gfx::DescriptorType::eStorageBuffer, 1, gfx::ShaderStageFlags_Compute },
										{ gfx::DescriptorType::eStorageBuffer, 1, gfx::ShaderStageFlags_Compute },
									} },
		},
	};
	gfx::PipelineHandle pipelineHandle{};
	if (!gfx::create_compute_pipeline(pipelineHandle, deviceHandle, pipelineInfo))
	{
		throw std::runtime_error("Failed to create GFX compute pipeline!");
	}

	gfx::BufferHandle inBufferHandle{};
	gfx::BufferHandle outBufferHandle{};
	gfx::BufferInfo bufferInfo{
		.type = gfx::BufferType::eStorage,
		.size = sizeof(int) * 10,
	};
	if (!gfx::create_buffer(inBufferHandle, deviceHandle, bufferInfo))
	{
		throw std::runtime_error("Failed to create GFX buffer!");
	}
	if (!gfx::create_buffer(outBufferHandle, deviceHandle, bufferInfo))
	{
		throw std::runtime_error("Failed to create GFX buffer!");
	}
	std::int32_t* inBufferPtr{ nullptr };
	if (gfx::map_buffer(inBufferHandle, reinterpret_cast<void*&>(inBufferPtr)))
	{
		for (auto i = 0; i < 10; ++i)
		{
			inBufferPtr[i] = i;
		}
		gfx::unmap_buffer(inBufferHandle);
	}

	gfx::DescriptorSetHandle descriptorSetHandle{};
	if (!gfx::create_descriptor_set_from_pipeline(descriptorSetHandle, pipelineHandle, 0))
	{
		throw std::runtime_error("Failed to create GFX descriptor set!");
	}
	gfx::bind_buffer_to_descriptor_set(descriptorSetHandle, 0, inBufferHandle);
	gfx::bind_buffer_to_descriptor_set(descriptorSetHandle, 1, outBufferHandle);

	gfx::CommandListHandle commandListHandle{};
	if (!gfx::create_command_list(commandListHandle, deviceHandle, 0))
	{
		throw std::runtime_error("Failed to create GFX command list!");
	}

	gfx::reset(commandListHandle);
	gfx::begin(commandListHandle);
	gfx::bind_pipeline(commandListHandle, pipelineHandle);
	gfx::bind_descriptor_set(commandListHandle, descriptorSetHandle);
	gfx::dispatch(commandListHandle, 10, 1, 1);
	gfx::end(commandListHandle);

	gfx::SubmitInfo submitInfo{
		.commandList = commandListHandle,
		.waitSemaphoreHandle = {}
	};
	gfx::FenceHandle fenceHandle;
	gfx::submit_command_list(submitInfo, &fenceHandle, nullptr);

	gfx::wait_on_fence(fenceHandle);

	if (gfx::map_buffer(inBufferHandle, reinterpret_cast<void*&>(inBufferPtr)))
	{
		for (auto i = 0; i < 10; ++i)
		{
			std::cout << inBufferPtr[i] << " ";
		}
		std::cout << std::endl;
		gfx::unmap_buffer(inBufferHandle);
	}
	std::int32_t* outBufferPtr{ nullptr };
	if (gfx::map_buffer(outBufferHandle, reinterpret_cast<void*&>(outBufferPtr)))
	{
		for (auto i = 0; i < 10; ++i)
		{
			std::cout << outBufferPtr[i] << " ";
		}
		std::cout << std::endl;
		gfx::unmap_buffer(outBufferHandle);
	}

	gfx::destroy_buffer(inBufferHandle);
	gfx::destroy_buffer(outBufferHandle);

	gfx::destroy_device(deviceHandle);
	gfx::shutdown();
}