/*
 * Copyright (c) Stuart Millman 2023.
 */

#include "gfx/gfx.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if _WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#else
	#error "Unsupported platform!"
#endif
#include <GLFW/glfw3native.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

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

struct UniformData
{
	glm::mat4 projMat;
	glm::mat4 viewMat;
};

int main()
{
	constexpr std::uint32_t WINDOW_WIDTH = 640;
	constexpr std::uint32_t WINDOW_HEIGHT = 480;
	constexpr float WINDOW_ASPECT_RATIO = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	auto* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Shader Uniforms", nullptr, nullptr);

	gfx::set_error_callback([](const char* msg) {
		GFX_LOG_ERR(msg);
		GFX_ASSERT(false, "");
	});

	gfx::AppInfo appInfo{ "Shader Uniforms App" };
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

	const auto vertShaderBinary = read_shader_file("uniforms.vert.spv");
	const auto fragShaderBinary = read_shader_file("uniforms.frag.spv");
	gfx::GraphicsPipelineInfo pipelineInfo{
		.vertexCode = vertShaderBinary,
		.fragmentCode = fragShaderBinary,
		.descriptorSets = {
			gfx::DescriptorSetInfo{ .bindings = {
										{ gfx::DescriptorType::eUniformBuffer, 1, gfx::ShaderStageFlags_Vertex },
									} },
		},
		.constantBlock = { sizeof(glm::mat4), gfx::ShaderStageFlags_Vertex },
	};
	gfx::PipelineHandle pipelineHandle{};
	if (!gfx::create_graphics_pipeline(pipelineHandle, deviceHandle, pipelineInfo))
	{
		throw std::runtime_error("Failed to create GFX graphics pipeline!");
	}

	gfx::BufferInfo uniformBufferInfo{
		.type = gfx::BufferType::eUniform,
		.size = sizeof(UniformData),
	};
	gfx::BufferHandle uniformBufferHandle{};
	if (!gfx::create_buffer(uniformBufferHandle, deviceHandle, uniformBufferInfo))
	{
		throw std::runtime_error("Failed to create GFX uniform buffer!");
	}

	UniformData uniformData{
		.projMat = glm::perspectiveLH(glm::radians(60.0f), WINDOW_ASPECT_RATIO, 0.1f, 100.0f),
		.viewMat = glm::lookAtLH(glm::vec3(-1, 2, -2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)),
	};
	void* bufferPtr{ nullptr };
	if (gfx::map_buffer(uniformBufferHandle, bufferPtr))
	{
		std::memcpy(bufferPtr, &uniformData, sizeof(UniformData));
		gfx::unmap_buffer(uniformBufferHandle);
	}

	gfx::DescriptorSetHandle descriptorSetHandle{};
	if (!gfx::create_descriptor_set_from_pipeline(descriptorSetHandle, pipelineHandle, 0))
	{
		throw std::runtime_error("Failed to create GFX descriptor set!");
	}
	gfx::bind_buffer_to_descriptor_set(descriptorSetHandle, 0, uniformBufferHandle);

	gfx::CommandListHandle commandListHandle{};
	if (!gfx::create_command_list(commandListHandle, deviceHandle, 0))
	{
		throw std::runtime_error("Failed to create GFX command list!");
	}

	double lastFrameTime = glfwGetTime();
	glm::mat4 modelMat = glm::mat4(1.0f);
	while (glfwWindowShouldClose(window) == 0)
	{
		auto time = glfwGetTime();
		auto deltaTime = static_cast<float>(time - lastFrameTime);
		lastFrameTime = time;

		glfwPollEvents();

		modelMat = glm::rotate(modelMat, glm::radians(45.0f) * deltaTime, glm::vec3(0, 1, 0));

		gfx::reset(commandListHandle);
		gfx::begin(commandListHandle);

		gfx::TextureHandle swapChainImageHandle{};
		if (!gfx::get_swap_chain_image(swapChainImageHandle, swapChainHandle))
		{
			throw std::runtime_error("Failed to get SwapChain image handle!");
		}

		gfx::transition_texture(commandListHandle, swapChainImageHandle, gfx::TextureState::eUndefined, gfx::TextureState::eRenderTarget);

		gfx::RenderPassInfo renderPassInfo{
			.colorAttachments = { swapChainImageHandle },
			.depthAttachment = {},
			.clearColor = { 0.392f, 0.584f, 0.929f, 1.0f }, // Cornflower Blue
		};
		gfx::begin_render_pass(commandListHandle, renderPassInfo);
		{
			gfx::set_viewport(commandListHandle, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
			gfx::set_scissor(commandListHandle, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

			gfx::bind_pipeline(commandListHandle, pipelineHandle);
			gfx::bind_descriptor_set(commandListHandle, descriptorSetHandle);
			gfx::set_constants(commandListHandle, gfx::ShaderStageFlags_Vertex, 0, sizeof(glm::mat4), glm::value_ptr(modelMat));

			gfx::draw(commandListHandle, 3, 1, 0, 0);
		}
		gfx::end_render_pass(commandListHandle);

		gfx::transition_texture(commandListHandle, swapChainImageHandle, gfx::TextureState::eRenderTarget, gfx::TextureState::ePresent);

		gfx::end(commandListHandle);

		gfx::SubmitInfo submitInfo{
			.commandList = commandListHandle,
			.waitSemaphoreHandle = {}
		};
		gfx::FenceHandle fenceHandle;
		gfx::submit_command_list(submitInfo, &fenceHandle, nullptr);

		gfx::wait_on_fence(fenceHandle);

		gfx::present_swap_chain(swapChainHandle, 0, nullptr);
	}

	gfx::destroy_swap_chain(swapChainHandle);

	gfx::destroy_device(deviceHandle);
	gfx::shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();
}