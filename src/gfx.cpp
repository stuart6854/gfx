/*
 * Copyright (c) Stuart Millman 2023.
 */

#include "gfx.hpp"
#include "gfx_p.hpp"

#include <memory>
#include <utility>
#include <functional>
#include <string_view>

#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace sm::gfx
{
	static std::function<void(const char* msg)> s_errorCallback;
	static std::unique_ptr<Context> s_context;

#pragma region Public Header

	void set_error_callback(std::function<void(const char* msg)> callback)
	{
		s_errorCallback = std::move(callback);
	}

	bool initialise(const AppInfo& appInfo)
	{
		if (s_context != nullptr)
		{
			return false;
		}

		s_context = std::make_unique<Context>(appInfo);
		if (!s_context->is_valid())
		{
			GFX_LOG_ERR("GFX - Failed to initialise context!");
			return false;
		}

		return true;
	}

	void shutdown()
	{
		s_context = nullptr;
	}

	bool create_device(DeviceHandle& outDeviceHandle, const DeviceInfo& deviceInfo)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		if (!s_context->create_device(outDeviceHandle, deviceInfo))
		{
			GFX_LOG_ERR("GFX - Failed to create device!");
			return false;
		}

		return true;
	}

	void destroy_device(DeviceHandle deviceHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		s_context->destroy_device(deviceHandle);
	}

#pragma endregion

#pragma region Utility

	static const std::unordered_map<TextureState, vk::ImageLayout> s_textureStateImageLayoutMap{
		{ TextureState::eUndefined, vk::ImageLayout::eUndefined },
		{ TextureState::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal },
		{ TextureState::eRenderTarget, vk::ImageLayout::eAttachmentOptimal },
		{ TextureState::ePresent, vk::ImageLayout::ePresentSrcKHR },
	};
	static const std::unordered_map<TextureState, vk::PipelineStageFlagBits2> s_barrierTextureStatePipelineStageMaskMap{
		{ TextureState::eUndefined, vk::PipelineStageFlagBits2::eTopOfPipe },
		{ TextureState::eShaderRead, vk::PipelineStageFlagBits2::eFragmentShader },
		{ TextureState::eRenderTarget, vk::PipelineStageFlagBits2::eColorAttachmentOutput },
		{ TextureState::ePresent, vk::PipelineStageFlagBits2::eBottomOfPipe },
	};
	static const std::unordered_map<TextureState, vk::AccessFlagBits2> s_barrierTextureStateAccessMaskMap{
		{ TextureState::eUndefined, vk::AccessFlagBits2::eNone },
		{ TextureState::eShaderRead, vk::AccessFlagBits2::eShaderRead },
		{ TextureState::eRenderTarget, vk::AccessFlagBits2::eColorAttachmentWrite },
		{ TextureState::ePresent, vk::AccessFlagBits2::eNone },
	};

	auto convert_shader_stages_to_vk_shader_stage_flags(std::uint32_t shaderStages) -> vk::ShaderStageFlags
	{
		vk::ShaderStageFlags stageFlags{};
		if (shaderStages & ShaderStageFlags_Compute)
		{
			stageFlags |= vk::ShaderStageFlagBits::eCompute;
		}
		if (shaderStages & ShaderStageFlags_Vertex)
		{
			stageFlags |= vk::ShaderStageFlagBits::eVertex;
		}
		if (shaderStages & ShaderStageFlags_Fragment)
		{
			stageFlags |= vk::ShaderStageFlagBits::eFragment;
		}
		// #TODO: Convert other shader stages.
		return stageFlags;
	}

	auto convert_format_to_vk_format(Format format) -> vk::Format
	{
		switch (format)
		{
			case Format::eUndefined:
				return vk::Format::eUndefined;
			case Format::eRGB8:
				return vk::Format::eR8G8B8Unorm;
			case Format::eRGB32:
				return vk::Format::eR32G32B32Sfloat;
			case Format::eRGBA8:
				return vk::Format::eR8G8B8A8Unorm;
			case Format::eRGBA32:
				return vk::Format::eR32G32B32A32Sfloat;
			default:
				GFX_ASSERT(false, "Cannot convert unknown Format to vk::Format!");
				break;
		}
		return vk::Format::eUndefined;
	}

	auto convert_format_to_byte_size(Format format) -> std::uint32_t
	{
		switch (format)
		{
			case Format::eUndefined:
				return 0;
			case Format::eRGB8:
				return 1 * 3;
			case Format::eRGBA8:
				return 1 * 4;
			case Format::eRGB32:
				return 4 * 3;
			case Format::eRGBA32:
				return 4 * 4;
			default:
				GFX_ASSERT(false, "Cannot convert unknown Format to byte size!");
				break;
		}
		return 0;
	}

	auto convert_buffer_type_to_vk_usage(BufferType bufferType) -> vk::BufferUsageFlags
	{
		switch (bufferType)
		{
			case BufferType::eVertex:
				return vk::BufferUsageFlagBits::eVertexBuffer;
			case BufferType::eIndex:
				return vk::BufferUsageFlagBits::eIndexBuffer;
			case BufferType::eUniform:
				return vk::BufferUsageFlagBits::eUniformBuffer;
			case BufferType::eStorage:
				return vk::BufferUsageFlagBits::eStorageBuffer;
			default:
				GFX_ASSERT(false, "Cannot convert unknown BufferType to vk::BufferUsageFlags!");
				break;
		}
		return {};
	}

	auto convert_buffer_type_to_descriptor_type(BufferType bufferType) -> vk::DescriptorType
	{
		switch (bufferType)
		{
			case BufferType::eUniform:
				return vk::DescriptorType::eUniformBuffer;
			case BufferType::eStorage:
				return vk::DescriptorType::eStorageBuffer;
			case BufferType::eVertex:
			case BufferType::eIndex:
				break; // These buffer types cannot be used in descriptors.
			default:
				GFX_ASSERT(false, "Cannot convert unknown BufferType to vk::DescriptorType!");
				break;
		}
		return {};
	}

#pragma endregion

#pragma region Device Resources

	void wait_on_fence(FenceHandle fenceHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, fenceHandle.deviceHandle))
		{
			s_errorCallback("gfx::wait_on_fence() - fenceHandle must be valid!");
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		device->wait_on_fence(fenceHandle);
	}

	bool create_command_list(CommandListHandle& outCommandListHandle, DeviceHandle deviceHandle, std::uint32_t queueIndex)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_command_list(outCommandListHandle, queueIndex);
	}

	void destroy_command_list(DeviceHandle deviceHandle, CommandListHandle commandListHandle)
	{
	}

	void submit_command_list(const SubmitInfo& submitInfo, FenceHandle* outFenceHandle, SemaphoreHandle* outSemaphoreHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, submitInfo.commandList.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		device->submit_command_list(submitInfo, outFenceHandle, outSemaphoreHandle);
	}

	bool create_compute_pipeline(PipelineHandle& outPipelineHandle, DeviceHandle deviceHandle, const ComputePipelineInfo& computePipelineInfo)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_compute_pipeline(outPipelineHandle, computePipelineInfo);
	}

	bool create_graphics_pipeline(PipelineHandle& outPipelineHandle, DeviceHandle deviceHandle, const GraphicsPipelineInfo& graphicsPipelineInfo)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_graphics_pipeline(outPipelineHandle, graphicsPipelineInfo);
	}

	void destroy_pipeline(PipelineHandle pipelineHandle)
	{
	}

	bool create_descriptor_set_from_pipeline(DescriptorSetHandle& outDescriptorSetHandle, PipelineHandle pipelineHandle, std::uint32_t set)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, pipelineHandle.deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_descriptor_set_from_pipeline(outDescriptorSetHandle, pipelineHandle, set);
	}

	void bind_buffer_to_descriptor_set(DescriptorSetHandle descriptorSetHandle, std::uint32_t binding, BufferHandle bufferHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");
		if (descriptorSetHandle.deviceHandle != bufferHandle.deviceHandle)
		{
			s_errorCallback("GFX - Buffer must belong to the same device as the descriptor set it is to be bound too!");
			return;
		}

		Device* device{ nullptr };
		if (!s_context->get_device(device, descriptorSetHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		device->bind_buffer_to_descriptor_set(descriptorSetHandle, binding, bufferHandle);
	}

	bool create_buffer(BufferHandle& outBufferHandle, DeviceHandle deviceHandle, const BufferInfo& bufferInfo)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_buffer(outBufferHandle, bufferInfo);
	}

	void destroy_buffer(BufferHandle bufferHandle)
	{
	}

	bool map_buffer(BufferHandle bufferHandle, void*& outBufferPtr)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, bufferHandle.deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->map_buffer(bufferHandle, outBufferPtr);
	}

	void unmap_buffer(BufferHandle bufferHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, bufferHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		device->unmap_buffer(bufferHandle);
	}

	bool create_texture(TextureHandle& outTextureHandle, DeviceHandle deviceHandle, const TextureInfo& textureInfo)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_texture(outTextureHandle, textureInfo);
	}

	void destroy_texture(TextureHandle textureHandle)
	{
	}

	bool create_swap_chain(SwapChainHandle& outSwapChainHandle, DeviceHandle deviceHandle, const SwapChainInfo& swapChainInfo)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_swap_chain(outSwapChainHandle, swapChainInfo);
	}

	void destroy_swap_chain(SwapChainHandle swapChainHandle)
	{
	}

	void present_swap_chain(SwapChainHandle swapChainHandle, std::uint32_t queueIndex, SemaphoreHandle* waitSemaphore)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, swapChainHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		vk::Queue queue{};
		if (!device->get_queue(queue, queueIndex))
		{
			return;
		}

		SwapChain* swapChain{ nullptr };
		if (!device->get_swap_chain(swapChain, swapChainHandle))
		{
			return;
		}

		vk::Semaphore wait_semaphore{};
		if (waitSemaphore != nullptr)
		{
		}

		swapChain->present(queue, wait_semaphore);
	}

	bool get_swap_chain_image(TextureHandle& outTextureHandle, SwapChainHandle swapChainHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, swapChainHandle.deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		SwapChain* swapChain{ nullptr };
		if (!device->get_swap_chain(swapChain, swapChainHandle))
		{
			return false;
		}

		outTextureHandle = swapChain->get_current_image_handle();
		return outTextureHandle.fullHandle > 0;
	}

#pragma endregion

#pragma region Command List Recording

	void reset(CommandListHandle commandListHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->reset();
	}

	bool begin(CommandListHandle commandListHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return false;
		}

		commandList->begin();

		return true;
	}

	void end(CommandListHandle commandListHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->end();
	}

	void begin_render_pass(CommandListHandle commandListHandle, const RenderPassInfo& renderPassInfo)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		std::vector<Texture*> colorAttachments(renderPassInfo.colorAttachments.size());
		for (auto i = 0; i < colorAttachments.size(); ++i)
		{
			auto success = device->get_texture(colorAttachments[i], renderPassInfo.colorAttachments[i]);
			GFX_ASSERT(success, "Failed to get Texture for color attachment from handle!");
		}

		Texture* depthAttachment{ nullptr };
		if (renderPassInfo.depthAttachment.fullHandle != 0)
		{
			auto success = device->get_texture(depthAttachment, renderPassInfo.depthAttachment);
			GFX_ASSERT(success, "Failed to get Texture for depth attachment from handle!");
		}

		commandList->begin_render_pass(colorAttachments, depthAttachment, renderPassInfo.clearColor);
	}

	void end_render_pass(CommandListHandle commandListHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->end_render_pass();
	}

	void set_viewport(CommandListHandle commandListHandle, float x, float y, float width, float height, float minDepth, float maxDepth)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->set_viewport(x, y, width, height, minDepth, maxDepth);
	}

	void set_scissor(CommandListHandle commandListHandle, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->set_scissor(x, y, width, height);
	}

	void bind_pipeline(CommandListHandle commandListHandle, PipelineHandle pipelineHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		Pipeline* pipeline{ nullptr };
		if (!device->get_pipeline(pipeline, pipelineHandle))
		{
			return;
		}
		GFX_ASSERT(pipeline != nullptr, "Pipeline should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->bind_pipeline(pipeline);
	}

	void bind_descriptor_set(CommandListHandle commandListHandle, DescriptorSetHandle descriptorSetHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		vk::DescriptorSet descriptorSet{};
		if (!device->get_descriptor_set(descriptorSet, descriptorSetHandle))
		{
			return;
		}

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->bind_descriptor_set(descriptorSet);
	}

	void set_constants(CommandListHandle commandListHandle, std::uint32_t shaderStages, std::uint32_t offset, std::uint32_t size, const void* data)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		auto vk_shader_stages = convert_shader_stages_to_vk_shader_stage_flags(shaderStages);
		commandList->set_constants(vk_shader_stages, offset, size, data);
	}

	void dispatch(CommandListHandle commandListHandle, std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void bind_index_buffer(CommandListHandle commandListHandle, BufferHandle bufferHandle, IndexType indexType)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		Buffer* buffer{ nullptr };
		if (!device->get_buffer(buffer, bufferHandle))
		{
			return;
		}

		auto vk_index_type = indexType == IndexType::eUInt16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
		commandList->bind_index_buffer(buffer, vk_index_type);
	}

	void bind_vertex_buffer(CommandListHandle commandListHandle, BufferHandle bufferHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		Buffer* buffer{ nullptr };
		if (!device->get_buffer(buffer, bufferHandle))
		{
			return;
		}

		commandList->bind_vertex_buffer(buffer);
	}

	void draw(CommandListHandle commandListHandle, std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex, std::uint32_t first_instance)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->draw(vertex_count, instance_count, first_vertex, first_instance);
	}

	void draw_indexed(CommandListHandle commandListHandle, std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->draw_indexed(index_count, instance_count, first_index, vertex_offset, first_instance);
	}

	void transition_texture(CommandListHandle commandListHandle, TextureHandle textureHandle, TextureState oldState, TextureState newState)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		Texture* texture{ nullptr };
		if (!device->get_texture(texture, textureHandle))
		{
			return;
		}

		CommandList* commandList{ nullptr };
		if (!device->get_command_list(commandList, commandListHandle))
		{
			return;
		}

		commandList->transition_texture(texture, oldState, newState);
	}

#pragma endregion

#pragma endregion

#pragma region Private Header

	Context::Context(const AppInfo& appInfo)
	{
		auto vkGetInstanceProcAddr = m_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

		vk::ApplicationInfo vk_app_info{};
		vk_app_info.setApiVersion(VK_API_VERSION_1_3);
		vk_app_info.setPApplicationName(appInfo.appName.c_str());

		std::vector<const char*> extensions = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_KHR_SURFACE_EXTENSION_NAME,
#if _WIN32
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
		};
		std::vector<const char*> layers = {
			"VK_LAYER_KHRONOS_validation",
		};

		vk::DebugUtilsMessengerCreateInfoEXT vk_debug_messenger_info{};
		vk_debug_messenger_info.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
		vk_debug_messenger_info.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk_debug_messenger_info.setPfnUserCallback(debug_utils_messenger_callback);

		vk::InstanceCreateInfo vk_inst_info{};
		vk_inst_info.setPApplicationInfo(&vk_app_info);
		vk_inst_info.setPEnabledExtensionNames(extensions);
		vk_inst_info.setPEnabledLayerNames(layers);
		vk_inst_info.setPNext(&vk_debug_messenger_info);
		m_instance = vk::createInstanceUnique(vk_inst_info);
		if (!*m_instance)
		{
			s_errorCallback("Failed to create Vulkan instance!");
			return;
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

		m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(vk_debug_messenger_info);
		if (!*m_debugMessenger)
		{
			s_errorCallback("Failed to create Vulkan debug messenger!");
		}
	}

	bool Context::is_valid() const
	{
		return *m_instance;
	}

	auto Context::create_device(DeviceHandle& outDeviceHandle, const DeviceInfo& deviceInfo) -> bool
	{
		auto deviceHandle = DeviceHandle(m_nextDeviceId);

		auto device = std::make_unique<Device>(*this, deviceHandle, deviceInfo);
		if (!device->is_valid())
		{
			return false;
		}

		m_nextDeviceId++;
		m_deviceMap[deviceHandle] = std::move(device);

		outDeviceHandle = deviceHandle;
		return true;
	}

	void Context::destroy_device(DeviceHandle deviceHandle)
	{
		if (!m_deviceMap.contains(deviceHandle))
			return;

		m_deviceMap.erase(deviceHandle);
	}

	bool Context::get_device(Device*& outDevice, DeviceHandle deviceHandle)
	{
		if (!m_deviceMap.contains(deviceHandle))
		{
			outDevice = nullptr;
			return false;
		}

		outDevice = m_deviceMap.at(deviceHandle).get();
		return true;
	}

	Device::Device(Context& context, DeviceHandle deviceHandle, const DeviceInfo& deviceInfo)
		: m_context(&context), m_deviceHandle(deviceHandle)
	{
		auto physicalDevices = m_context->get_instance().enumeratePhysicalDevices();
		if (physicalDevices.empty())
		{
			s_errorCallback("GFX - There are no devices!");
			return;
		}

		std::uint32_t bestScore = 0;
		std::uint32_t bestDevice = 0;
		for (auto i = 0; i < physicalDevices.size(); ++i)
		{
			const auto& physicalDevice = physicalDevices[i];
			const auto& deviceProperties = physicalDevice.getProperties();

			std::uint32_t score = 0;

			if (deviceInfo.deviceFlags & DeviceFlags_PreferIntegrated && deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			{
				score += 1000;
			}
			if (deviceInfo.deviceFlags & DeviceFlags_PreferDiscrete && deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				score += 1000;
			}

			score += deviceProperties.limits.maxImageDimension2D;
			score += deviceProperties.limits.maxDescriptorSetSampledImages;
			score += deviceProperties.limits.maxDescriptorSetUniformBuffers;
			score += deviceProperties.limits.maxBoundDescriptorSets;

			if (score > bestScore)
			{
				bestScore = score;
				bestDevice = i;
			}
		}

		m_physicalDevice = physicalDevices[bestDevice];

		std::vector<const char*> extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		};

		auto queueProperties = m_physicalDevice.getQueueFamilyProperties();

		m_queueFlags = deviceInfo.queueFlags;
		m_queueFamilies.resize(m_queueFlags.size());
		m_queues.resize(m_queueFlags.size());

		std::unordered_map<std::uint32_t, std::uint32_t> usedQueueFamilyCounts;
		for (auto i = 0; i < deviceInfo.queueFlags.size(); ++i)
		{
			const auto queueFlags = deviceInfo.queueFlags[i];

			vk::QueueFlags wantedFlags;
			if (queueFlags & QueueFlags_Graphics)
			{
				wantedFlags |= vk::QueueFlagBits::eGraphics;
			}
			if (queueFlags & QueueFlags_Compute)
			{
				wantedFlags |= vk::QueueFlagBits::eCompute;
			}
			if (queueFlags & QueueFlags_Transfer)
			{
				wantedFlags |= vk::QueueFlagBits::eTransfer;
			}

			for (auto familyIndex = 0; familyIndex < queueProperties.size(); ++familyIndex)
			{
				auto& queueProps = queueProperties[familyIndex];
				if (queueProps.queueFlags & wantedFlags)
				{
					usedQueueFamilyCounts[familyIndex] += 1;
					m_queueFamilies[i] = familyIndex;
					break;
				}
			}
		}

		const float QUEUE_PRIORITY = 1.0f;
		std::vector<vk::DeviceQueueCreateInfo> queue_info_vec;
		for (auto [family, count] : usedQueueFamilyCounts)
		{
			auto& queue_info = queue_info_vec.emplace_back();
			queue_info.setQueueFamilyIndex(family);
			queue_info.setQueueCount(count);
			queue_info.setPQueuePriorities(&QUEUE_PRIORITY);
		}

		vk::PhysicalDeviceFeatures features{};
		vk::PhysicalDeviceSynchronization2Features sync_2_features{ true };
		vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{ true, &sync_2_features };

		vk::DeviceCreateInfo vk_device_info{};
		vk_device_info.setPEnabledExtensionNames(extensions);
		vk_device_info.setQueueCreateInfos(queue_info_vec);
		vk_device_info.setPEnabledFeatures(&features);
		vk_device_info.setPNext(&dynamic_rendering_features);
		m_device = m_physicalDevice.createDeviceUnique(vk_device_info);
		if (!*m_device)
		{
			s_errorCallback("GFX - Failed to create device!");
			return;
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);

		std::unordered_map<std::uint32_t, std::uint32_t> queueIndexMap;
		for (auto i = 0; i < m_queueFamilies.size(); ++i)
		{
			auto queueFamily = m_queueFamilies[i];

			vk::CommandPoolCreateInfo cmd_pool_info{};
			cmd_pool_info.setQueueFamilyIndex(queueFamily);
			cmd_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
			m_queueFamilyCommandPoolMap[queueFamily] = m_device->createCommandPoolUnique(cmd_pool_info);

			auto queueIndex = queueIndexMap[queueFamily];
			m_queues[i] = m_device->getQueue(queueFamily, queueIndex);
			queueIndexMap[queueFamily] += 1;
		}

		vma::AllocatorCreateInfo allocator_info{};
		allocator_info.setInstance(m_context->get_instance());
		allocator_info.setPhysicalDevice(m_physicalDevice);
		allocator_info.setDevice(m_device.get());
		allocator_info.setVulkanApiVersion(VK_API_VERSION_1_3);
		m_allocator = vma::createAllocatorUnique(allocator_info);

		const std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes{
			{ vk::DescriptorType::eStorageBuffer, 100 },
			{ vk::DescriptorType::eUniformBuffer, 100 },
			{ vk::DescriptorType::eCombinedImageSampler, 100 },
		};
		vk::DescriptorPoolCreateInfo descriptor_pool_info{};
		descriptor_pool_info.setMaxSets(100);
		descriptor_pool_info.setPoolSizes(descriptor_pool_sizes);
		descriptor_pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
		m_descriptorPool = m_device->createDescriptorPoolUnique(descriptor_pool_info);
	}

	bool Device::is_valid() const
	{
		return static_cast<bool>(*m_device);
	}

	bool Device::get_queue(vk::Queue& outQueue, std::uint32_t queueIndex)
	{
		if (queueIndex > m_queues.size())
		{
			return false;
		}

		outQueue = m_queues.at(queueIndex);
		return outQueue;
	}

	bool Device::is_present_mode_supported(vk::PresentModeKHR presentMode, vk::SurfaceKHR surface) const
	{
		auto supportedPresentModes = m_physicalDevice.getSurfacePresentModesKHR(surface);
		return std::ranges::find(supportedPresentModes, presentMode) != supportedPresentModes.end();
	}

	auto Device::get_first_supported_surface_format(const std::vector<vk::Format>& formats, vk::SurfaceKHR surface) -> vk::Format
	{
		const auto supportedSurfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(surface);
		for (const auto& format : formats)
		{
			for (const auto& surfaceFormat : supportedSurfaceFormats)
			{
				if (surfaceFormat.format == format)
					return format;
			}
		}

		return vk::Format::eUndefined;
	}

	void Device::wait_on_fence(FenceHandle fenceHandle)
	{
		vk::Fence fence = m_fenceMap.at(fenceHandle.resourceHandle).get();
		auto result = m_device->waitForFences(fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
		GFX_UNUSED(result);

		m_fenceMap.erase(fenceHandle.resourceHandle);
	}

	auto Device::create_command_list(CommandListHandle& outCommandListHandle, std::uint32_t queueIndex) -> bool
	{
		auto queueFamily = m_queueFamilies.at(queueIndex);
		auto commandPool = m_queueFamilyCommandPoolMap.at(queueFamily).get();
		auto queue = m_queues.at(queueIndex);

		CommandListHandle commandListHandle(m_deviceHandle, ResourceHandle(m_nextCommandListId));

		m_commandListMap[commandListHandle.resourceHandle] = std::make_unique<CommandList>(m_device.get(), commandPool, queue);
		m_nextCommandListId += 1;

		outCommandListHandle = commandListHandle;
		return true;
	}

	bool Device::get_command_list(CommandList*& outCommandList, CommandListHandle commandListHandle)
	{
		if (!m_commandListMap.contains(commandListHandle.resourceHandle))
		{
			return false;
		}

		outCommandList = m_commandListMap.at(commandListHandle.resourceHandle).get();
		return true;
	}

	bool Device::create_or_get_descriptor_set_layout(vk::DescriptorSetLayout& outDescriptorSetLayout, const DescriptorSetInfo& descriptorSetInfo)
	{
		const auto hash = std::hash<DescriptorSetInfo>{}(descriptorSetInfo);
		if (!m_descriptorSetLayoutMap.contains(hash))
		{
			std::vector<vk::DescriptorSetLayoutBinding> vk_bindings(descriptorSetInfo.bindings.size());
			for (auto i = 0; i < vk_bindings.size(); ++i)
			{
				vk_bindings[i] = get_descriptor_set_layout_binding(descriptorSetInfo.bindings.at(i));
				vk_bindings[i].setBinding(i);
			}

			vk::DescriptorSetLayoutCreateInfo set_layout_info{};
			set_layout_info.setBindings(vk_bindings);
			m_descriptorSetLayoutMap[hash] = m_device->createDescriptorSetLayoutUnique(set_layout_info);
		}

		outDescriptorSetLayout = m_descriptorSetLayoutMap.at(hash).get();
		return true;
	}

	bool Device::submit_command_list(const SubmitInfo& submitInfo, FenceHandle* outFenceHandle, SemaphoreHandle* outSemaphoreHandle)
	{
		if (!m_commandListMap.contains(submitInfo.commandList.resourceHandle))
		{
			return false;
		}

		vk::Fence fence{};
		if (outFenceHandle != nullptr)
		{
			*outFenceHandle = create_fence();
			fence = m_fenceMap.at(outFenceHandle->resourceHandle).get();
		}

		vk::Semaphore signalSemaphore{};
		if (outSemaphoreHandle != nullptr)
		{
			// TODO: Create signalSemaphore
		}

		const auto& command_list = m_commandListMap.at(submitInfo.commandList.resourceHandle);
		auto command_buffer = command_list->get_command_buffer();

		vk::SubmitInfo submit_info{};
		submit_info.setCommandBuffers(command_buffer);
		if (signalSemaphore)
		{
			submit_info.setSignalSemaphores(signalSemaphore);
		}

		auto queue = command_list->get_queue();
		queue.submit(submit_info, fence);

		return true;
	}

	bool Device::create_compute_pipeline(PipelineHandle& outPipelineHandle, const ComputePipelineInfo& computePipelineInfo)
	{
		PipelineHandle pipelineHandle(m_deviceHandle, ResourceHandle(m_nextPipelineId));

		std::vector<vk::DescriptorSetLayout> setLayouts(computePipelineInfo.descriptorSets.size());
		for (auto i = 0; i < setLayouts.size(); ++i)
		{
			vk::DescriptorSetLayout setLayout{};
			if (create_or_get_descriptor_set_layout(setLayout, computePipelineInfo.descriptorSets.at(i)))
			{
				setLayouts[i] = setLayout;
			}
		}

		vk::PushConstantRange constantRange{
			convert_shader_stages_to_vk_shader_stage_flags(computePipelineInfo.constantBlock.shaderStages),
			0,
			computePipelineInfo.constantBlock.size
		};
		m_pipelineMap[pipelineHandle.resourceHandle] = std::make_unique<ComputePipeline>(m_device.get(), computePipelineInfo.shaderCode, setLayouts, constantRange);
		m_nextPipelineId += 1;

		outPipelineHandle = pipelineHandle;
		return true;
	}

	bool Device::create_graphics_pipeline(PipelineHandle& outPipelineHandle, const GraphicsPipelineInfo& graphicsPipelineInfo)
	{
		PipelineHandle pipelineHandle(m_deviceHandle, ResourceHandle(m_nextPipelineId));

		std::vector<vk::DescriptorSetLayout> setLayouts(graphicsPipelineInfo.descriptorSets.size());
		for (auto i = 0; i < setLayouts.size(); ++i)
		{
			vk::DescriptorSetLayout setLayout{};
			if (create_or_get_descriptor_set_layout(setLayout, graphicsPipelineInfo.descriptorSets.at(i)))
			{
				setLayouts[i] = setLayout;
			}
		}

		vk::PushConstantRange constantRange{
			convert_shader_stages_to_vk_shader_stage_flags(graphicsPipelineInfo.constantBlock.shaderStages),
			0,
			graphicsPipelineInfo.constantBlock.size
		};
		m_pipelineMap[pipelineHandle.resourceHandle] = std::make_unique<GraphicsPipeline>(m_device.get(), graphicsPipelineInfo.vertexCode, graphicsPipelineInfo.fragmentCode, graphicsPipelineInfo.vertexAttributes, setLayouts, constantRange);
		m_nextPipelineId += 1;

		outPipelineHandle = pipelineHandle;
		return true;
	}

	void Device::destroy_pipeline(PipelineHandle pipelineHandle)
	{
	}

	bool Device::get_pipeline(Pipeline*& outPipeline, PipelineHandle pipelineHandle)
	{
		if (!m_pipelineMap.contains(pipelineHandle.resourceHandle))
		{
			outPipeline = nullptr;
			return false;
		}

		outPipeline = m_pipelineMap.at(pipelineHandle.resourceHandle).get();
		return true;
	}

	bool Device::create_descriptor_set_from_pipeline(DescriptorSetHandle& outDescriptorSetHandle, PipelineHandle pipelineHandle, std::uint32_t set)
	{
		Pipeline* pipeline{ nullptr };
		if (!get_pipeline(pipeline, pipelineHandle))
		{
			return false;
		}

		auto descriptorSetLayout = pipeline->get_set_layout(set);

		DescriptorSetHandle descriptorSetHandle(m_deviceHandle, ResourceHandle(m_nextDescriptorSetId));

		vk::DescriptorSetAllocateInfo set_alloc_info{};
		set_alloc_info.setDescriptorPool(m_descriptorPool.get());
		set_alloc_info.setSetLayouts(descriptorSetLayout);
		auto allocatedDescriptorSets = m_device->allocateDescriptorSetsUnique(set_alloc_info);

		m_descriptorSetMap[descriptorSetHandle.resourceHandle] = std::move(allocatedDescriptorSets[0]);
		m_nextDescriptorSetId += 1;

		outDescriptorSetHandle = descriptorSetHandle;
		return true;
	}

	bool Device::get_descriptor_set(vk::DescriptorSet& outDescriptorSet, DescriptorSetHandle descriptorSetHandle)
	{
		if (!m_descriptorSetMap.contains(descriptorSetHandle.resourceHandle))
		{
			outDescriptorSet = nullptr;
			return false;
		}

		outDescriptorSet = m_descriptorSetMap.at(descriptorSetHandle.resourceHandle).get();
		return true;
	}

	void Device::bind_buffer_to_descriptor_set(DescriptorSetHandle descriptorSetHandle, std::uint32_t binding, BufferHandle bufferHandle)
	{
		if (!m_descriptorSetMap.contains(descriptorSetHandle.resourceHandle))
		{
			s_errorCallback("GFX - Cannot bind buffer to unknown descriptor set!");
			return;
		}
		const auto descriptorSet = m_descriptorSetMap.at(descriptorSetHandle.resourceHandle).get();

		if (!m_bufferMap.contains(bufferHandle.resourceHandle))
		{
			s_errorCallback("GFX - Cannot bind unknown buffer to descriptor set!");
			return;
		}
		const auto& buffer = m_bufferMap.at(bufferHandle.resourceHandle);

		vk::WriteDescriptorSet write{};
		write.setDstSet(descriptorSet);
		write.setDstBinding(binding);
		write.setDescriptorCount(1);
		write.setDescriptorType(buffer->get_descriptor_type());
		write.setBufferInfo(buffer->get_descriptor_info());

		m_device->updateDescriptorSets(write, {});
	}

	bool Device::create_buffer(BufferHandle& outBufferHandle, const BufferInfo& bufferInfo)
	{
		BufferHandle bufferHandle(m_deviceHandle, ResourceHandle(m_nextBufferId));

		m_bufferMap[bufferHandle.resourceHandle] = std::make_unique<Buffer>(m_device.get(), m_allocator.get(), bufferInfo);
		m_nextBufferId += 1;

		outBufferHandle = bufferHandle;
		return true;
	}

	void Device::destroy_buffer(BufferHandle bufferHandle)
	{
	}

	bool Device::get_buffer(Buffer*& outBuffer, BufferHandle bufferHandle)
	{
		if (!m_bufferMap.contains(bufferHandle.resourceHandle))
		{
			outBuffer = nullptr;
			return false;
		}

		outBuffer = m_bufferMap.at(bufferHandle.resourceHandle).get();
		return true;
	}

	bool Device::map_buffer(BufferHandle bufferHandle, void*& outBufferPtr)
	{
		if (!m_bufferMap.contains(bufferHandle.resourceHandle))
		{
			return false;
		}

		const auto& buffer = m_bufferMap.at(bufferHandle.resourceHandle);

		outBufferPtr = m_allocator->mapMemory(buffer->get_allocation());
		return outBufferPtr != nullptr;
	}

	void Device::unmap_buffer(BufferHandle bufferHandle)
	{
		if (!m_bufferMap.contains(bufferHandle.resourceHandle))
		{
			return;
		}

		const auto& buffer = m_bufferMap.at(bufferHandle.resourceHandle);
		m_allocator->unmapMemory(buffer->get_allocation());
	}

	bool Device::create_texture(TextureHandle& outTextureHandle, const TextureInfo& textureInfo)
	{
		TextureHandle textureHandle(m_deviceHandle, ResourceHandle(m_nextTextureId));

		m_textureMap[textureHandle.resourceHandle] = std::make_unique<Texture>(*this, textureInfo);
		m_nextTextureId += 1;

		outTextureHandle = textureHandle;
		return true;
	}

	bool Device::create_texture(TextureHandle& outTextureHandle, vk::Image image, vk::Extent3D extent, vk::Format format)
	{
		TextureHandle textureHandle(m_deviceHandle, ResourceHandle(m_nextTextureId));

		m_textureMap[textureHandle.resourceHandle] = std::make_unique<Texture>(*this, image, extent, format);
		m_nextTextureId += 1;

		outTextureHandle = textureHandle;
		return true;
	}

	void Device::destroy_texture(TextureHandle textureHandle)
	{
	}

	bool Device::get_texture(Texture*& outTexture, TextureHandle textureHandle)
	{
		if (!m_textureMap.contains(textureHandle.resourceHandle))
		{
			return false;
		}

		outTexture = m_textureMap.at(textureHandle.resourceHandle).get();
		return true;
	}

	bool Device::create_swap_chain(SwapChainHandle& outSwapChainHandle, const SwapChainInfo& swapChainInfo)
	{
		SwapChainHandle swapChainHandle(m_deviceHandle, ResourceHandle(m_nextSwapChainId));

		m_swapChainMap[swapChainHandle.resourceHandle] = std::make_unique<SwapChain>(*this, swapChainInfo);
		m_nextSwapChainId += 1;

		outSwapChainHandle = swapChainHandle;
		return true;
	}

	void Device::destroy_swap_chain(SwapChainHandle swapChainHandle)
	{
	}

	bool Device::get_swap_chain(SwapChain*& outSwapChain, SwapChainHandle swapChainHandle)
	{
		if (!m_swapChainMap.contains(swapChainHandle.resourceHandle))
		{
			outSwapChain = nullptr;
			return false;
		}

		outSwapChain = m_swapChainMap.at(swapChainHandle.resourceHandle).get();
		return true;
	}

	auto Device::create_fence() -> FenceHandle
	{
		auto fenceHandle = FenceHandle(m_deviceHandle, ResourceHandle(m_nextFenceId));

		vk::FenceCreateInfo fence_info{};
		m_fenceMap[fenceHandle.resourceHandle] = m_device->createFenceUnique(fence_info);

		m_nextFenceId += 1;
		return fenceHandle;
	}

	auto Device::get_descriptor_set_layout_binding(const DescriptorBindingInfo& descriptorBindingInfo) -> vk::DescriptorSetLayoutBinding
	{
		vk::DescriptorSetLayoutBinding outBinding{};

		switch (descriptorBindingInfo.type)
		{
			case DescriptorType::eStorageBuffer:
				outBinding.setDescriptorType(vk::DescriptorType::eStorageBuffer);
				break;
			case DescriptorType::eUniformBuffer:
				outBinding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
				break;
		}

		outBinding.setDescriptorCount(descriptorBindingInfo.count);

		auto stageFlags = convert_shader_stages_to_vk_shader_stage_flags(descriptorBindingInfo.shaderStages);
		outBinding.setStageFlags(stageFlags);

		return outBinding;
	}

	CommandList::CommandList(vk::Device device, vk::CommandPool commandPool, vk::Queue queue)
		: m_device(device), m_commandPool(commandPool), m_queue(queue)
	{
		vk::CommandBufferAllocateInfo cmd_alloc_info{};
		cmd_alloc_info.setCommandPool(m_commandPool);
		cmd_alloc_info.setCommandBufferCount(1);
		cmd_alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
		m_commandBuffer = std::move(m_device.allocateCommandBuffersUnique(cmd_alloc_info)[0]);
	}

	CommandList::CommandList(CommandList&& other) noexcept
	{
		std::swap(m_device, other.m_device);
		std::swap(m_commandPool, other.m_commandPool);
		std::swap(m_queue, other.m_queue);
		std::swap(m_commandBuffer, other.m_commandBuffer);
		std::swap(m_hasBegun, other.m_hasBegun);
	}

	bool CommandList::is_valid() const
	{
		return static_cast<bool>(*m_commandBuffer);
	}

	void CommandList::reset()
	{
		m_commandBuffer->reset();
		m_hasBegun = false;
		m_boundPipeline = nullptr;
	}

	void CommandList::begin()
	{
		if (m_hasBegun)
		{
			s_errorCallback("GFX - CommandList has already begun recording!");
			return;
		}

		vk::CommandBufferBeginInfo cmd_begin_info{};
		m_commandBuffer->begin(cmd_begin_info);
		m_hasBegun = true;
	}

	void CommandList::end()
	{
		if (!m_hasBegun)
		{
			s_errorCallback("GFX - Cannot end() CommandList that has not even begun!");
			return;
		}

		m_commandBuffer->end();
	}

	void CommandList::begin_render_pass(const std::vector<Texture*>& colorAttachmentTextures, Texture* depthAttachmentTexture, const std::array<float, 4>& clearColor)
	{
		if (!m_hasBegun)
		{
			return;
		}

		std::vector<vk::RenderingAttachmentInfo> colorAttachments(colorAttachmentTextures.size());
		for (auto i = 0; i < colorAttachments.size(); ++i)
		{
			auto* texture = colorAttachmentTextures[i];
			auto& attachment = colorAttachments[i];
			attachment.setImageView(texture->get_view());
			attachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
			attachment.setLoadOp(vk::AttachmentLoadOp::eClear); // #TODO: Optional.
			attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
			attachment.setClearValue(vk::ClearColorValue(clearColor));
		}

		vk::RenderingAttachmentInfo depthAttachment{};
		if (depthAttachmentTexture != nullptr)
		{
			depthAttachment.setImageView({});									// #TODO: Image views
			depthAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
			depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);			// #TODO: Optional.
			depthAttachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
			depthAttachment.setClearValue(vk::ClearDepthStencilValue(1.0f, 0)); // #TODO: Optional.
		}

		auto textureExtent = colorAttachmentTextures.front()->get_extent();
		vk::Rect2D renderArea{ vk::Offset2D{ 0, 0 }, vk::Extent2D{ textureExtent.width, textureExtent.height } };

		vk::RenderingInfo rendering_info{};
		rendering_info.setLayerCount(1);
		rendering_info.setColorAttachments(colorAttachments);
		if (depthAttachmentTexture != nullptr)
		{
			rendering_info.setPDepthAttachment(&depthAttachment);
		}
		rendering_info.setRenderArea(renderArea);

		m_commandBuffer->beginRendering(rendering_info);
	}

	void CommandList::end_render_pass()
	{
		if (!m_hasBegun)
		{
			return;
		}

		m_commandBuffer->endRendering();
	}

	void CommandList::set_viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
	{
		if (!m_hasBegun)
		{
			return;
		}

		vk::Viewport viewport{ x, y + height, width, -height, minDepth, maxDepth };
		m_commandBuffer->setViewport(0, viewport);
	}

	void CommandList::set_scissor(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height)
	{
		if (!m_hasBegun)
		{
			return;
		}

		vk::Rect2D scissor{ { x, y }, { width, height } };
		m_commandBuffer->setScissor(0, scissor);
	}

	void CommandList::bind_pipeline(Pipeline* pipeline)
	{
		if (pipeline == nullptr)
		{
			s_errorCallback("GFX - Command list cannot bind null pipeline!");
			return;
		}

		const vk::PipelineBindPoint bindPoint = pipeline->get_type() == PipelineType::eCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics;
		m_commandBuffer->bindPipeline(bindPoint, pipeline->get_pipeline());

		m_boundPipeline = pipeline;
	}

	void CommandList::bind_descriptor_set(vk::DescriptorSet descriptorSet)
	{
		if (!m_hasBegun)
		{
			return;
		}
		if (m_boundPipeline == nullptr)
		{
			s_errorCallback("GFX - Cannot bind descriptor set when no pipeline has been bound!");
			return;
		}

		const vk::PipelineBindPoint bindPoint = m_boundPipeline->get_type() == PipelineType::eCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics;
		const auto pipelineLayout = m_boundPipeline->get_pipeline_layout();
		m_commandBuffer->bindDescriptorSets(bindPoint, pipelineLayout, 0, { descriptorSet }, {});
	}

	void CommandList::set_constants(vk::ShaderStageFlags shaderStages, std::uint32_t offset, std::uint32_t size, const void* data)
	{
		if (!m_hasBegun)
		{
			return;
		}

		const auto pipelineLayout = m_boundPipeline->get_pipeline_layout();
		m_commandBuffer->pushConstants(pipelineLayout, shaderStages, offset, size, data);
	}

	void CommandList::dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ)
	{
		if (!m_hasBegun)
		{
			return;
		}

		m_commandBuffer->dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void CommandList::bind_index_buffer(Buffer* buffer, vk::IndexType indexType)
	{
		if (!m_hasBegun)
		{
			return;
		}

		m_commandBuffer->bindIndexBuffer(buffer->get_buffer(), 0, indexType);
	}

	void CommandList::bind_vertex_buffer(Buffer* buffer)
	{
		if (!m_hasBegun)
		{
			return;
		}

		m_commandBuffer->bindVertexBuffers(0, buffer->get_buffer(), { 0 });
	}

	void CommandList::draw(std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex, std::uint32_t first_instance)
	{
		if (!m_hasBegun)
		{
			return;
		}

		m_commandBuffer->draw(vertex_count, instance_count, first_vertex, first_instance);
	}

	void CommandList::draw_indexed(std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance)
	{
		if (!m_hasBegun)
		{
			return;
		}

		m_commandBuffer->drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
	}

	void CommandList::transition_texture(Texture* texture, TextureState oldState, TextureState newState)
	{
		if (!m_hasBegun)
		{
			return;
		}

		GFX_ASSERT(s_textureStateImageLayoutMap.contains(oldState) && s_textureStateImageLayoutMap.contains(newState), "Unable to convert TextureState to vk::ImageLayout for barrier!");
		GFX_ASSERT(s_barrierTextureStatePipelineStageMaskMap.contains(oldState) && s_barrierTextureStatePipelineStageMaskMap.contains(newState), "Unable to convert TextureState to vk::PipelineStage for barrier!");
		GFX_ASSERT(s_barrierTextureStateAccessMaskMap.contains(oldState) && s_barrierTextureStateAccessMaskMap.contains(newState), "Unable to convert TextureState to vk::Access for barrier!");

		vk::ImageSubresourceRange range{};
		range.setAspectMask(vk::ImageAspectFlagBits::eColor); // #TODO: Get from texture.
		range.setBaseArrayLayer(0);							  // #TODO: Optional.
		range.setLayerCount(1);								  // #TODO: Optional.
		range.setBaseMipLevel(0);							  // #TODO: Optional.
		range.setLevelCount(1);								  // #TODO: Optional.

		auto srcLayout = s_textureStateImageLayoutMap.at(oldState);
		auto dstLayout = s_textureStateImageLayoutMap.at(newState);

		auto srcStage = s_barrierTextureStatePipelineStageMaskMap.at(oldState);
		auto dstStage = s_barrierTextureStatePipelineStageMaskMap.at(newState);

		auto srcAccess = s_barrierTextureStateAccessMaskMap.at(oldState);
		auto dstAccess = s_barrierTextureStateAccessMaskMap.at(newState);

		vk::ImageMemoryBarrier2 barrier{};
		barrier.setImage(texture->get_image());
		barrier.setOldLayout(srcLayout);
		barrier.setNewLayout(dstLayout);
		barrier.setSrcStageMask(srcStage);
		barrier.setDstStageMask(dstStage);
		barrier.setSrcAccessMask(srcAccess);
		barrier.setDstAccessMask(dstAccess);
		barrier.setSubresourceRange(range);

		vk::DependencyInfo dependency_info{};
		dependency_info.setImageMemoryBarriers(barrier);

		m_commandBuffer->pipelineBarrier2(dependency_info);
	}

	auto CommandList::operator=(CommandList&& rhs) noexcept -> CommandList&
	{
		std::swap(m_device, rhs.m_device);
		std::swap(m_commandPool, rhs.m_commandPool);
		std::swap(m_queue, rhs.m_queue);
		std::swap(m_commandBuffer, rhs.m_commandBuffer);
		std::swap(m_hasBegun, rhs.m_hasBegun);
		return *this;
	}

	Buffer::Buffer(vk::Device device, vma::Allocator allocator, const BufferInfo& bufferInfo)
		: m_device(device), m_allocator(allocator)
	{
		vk::BufferCreateInfo vk_buffer_info{};
		vk_buffer_info.setUsage(convert_buffer_type_to_vk_usage(bufferInfo.type));
		vk_buffer_info.setSize(bufferInfo.size);
		vk_buffer_info.setSharingMode(vk::SharingMode::eExclusive);
		//		vk_buffer_info.setQueueFamilyIndices(); // #TODO: Add later?

		vma::AllocationCreateInfo alloc_info{};
		alloc_info.setUsage(vma::MemoryUsage::eAutoPreferDevice);						// #TODO: Make optional.
		alloc_info.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite); // #TODO: Optional.

		std::tie(m_buffer, m_allocation) = m_allocator.createBufferUnique(vk_buffer_info, alloc_info);

		m_descriptorType = convert_buffer_type_to_descriptor_type(bufferInfo.type);

		m_descriptorInfo.setBuffer(m_buffer.get());
		m_descriptorInfo.setOffset(0);
		m_descriptorInfo.setRange(bufferInfo.size);
	}

	Buffer::Buffer(Buffer&& other) noexcept
	{
		std::swap(m_device, other.m_device);
		std::swap(m_allocation, other.m_allocation);
		std::swap(m_buffer, other.m_buffer);
		std::swap(m_allocation, other.m_allocation);
		std::swap(m_descriptorType, other.m_descriptorType);
		std::swap(m_descriptorInfo, other.m_descriptorInfo);
	}

	auto Buffer::operator=(Buffer&& rhs) noexcept -> Buffer&
	{
		std::swap(m_device, rhs.m_device);
		std::swap(m_allocation, rhs.m_allocation);
		std::swap(m_buffer, rhs.m_buffer);
		std::swap(m_allocation, rhs.m_allocation);
		std::swap(m_descriptorType, rhs.m_descriptorType);
		std::swap(m_descriptorInfo, rhs.m_descriptorInfo);
		return *this;
	}

	Texture::Texture(Device& device, const TextureInfo& textureInfo)
		: m_device(&device)
	{
		m_extent = vk::Extent3D(textureInfo.width, textureInfo.height, 1);

		vk::ImageCreateInfo image_info{};
		image_info.setExtent(m_extent);
		image_info.setMipLevels(m_mipLevels);
		image_info.setFormat(m_format);
		image_info.setUsage(m_usageFlags);
		image_info.setImageType(m_type);
		image_info.setArrayLayers(1);						// #TODO: Optional.
		image_info.setTiling(vk::ImageTiling::eOptimal);
		image_info.setSamples(vk::SampleCountFlagBits::e1); // #TODO: Optional.

		vma::AllocationCreateInfo alloc_info{};
		alloc_info.setUsage(vma::MemoryUsage::eAutoPreferDevice);						// #TODO: Make optional.
		alloc_info.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite); // #TODO: Optional.

		auto allocator = m_device->get_allocator();
		std::tie(m_image, m_allocation) = allocator.createImage(image_info, alloc_info);

		vk::ImageViewCreateInfo view_info{};
		view_info.setImage(m_image);
		view_info.setFormat(m_format);
		view_info.setViewType(vk::ImageViewType::e2D);
		view_info.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
		view_info.subresourceRange.setBaseMipLevel(0);
		view_info.subresourceRange.setLevelCount(1);
		view_info.subresourceRange.setBaseArrayLayer(0);
		view_info.subresourceRange.setLayerCount(1);
		m_view = m_device->get_device().createImageViewUnique(view_info);
	}

	Texture::Texture(Device& device, vk::Image image, vk::Extent3D extent, vk::Format format)
		: m_device(&device), m_image(image), m_extent(extent), m_format(format)
	{
		vk::ImageViewCreateInfo view_info{};
		view_info.setImage(m_image);
		view_info.setFormat(m_format);
		view_info.setViewType(vk::ImageViewType::e2D);
		view_info.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
		view_info.subresourceRange.setBaseMipLevel(0);
		view_info.subresourceRange.setLevelCount(1);
		view_info.subresourceRange.setBaseArrayLayer(0);
		view_info.subresourceRange.setLayerCount(1);
		m_view = m_device->get_device().createImageViewUnique(view_info);
	}

	Texture::Texture(Texture&& other) noexcept
	{
		std::swap(m_device, other.m_device);
		std::swap(m_image, other.m_image);
		std::swap(m_allocation, other.m_allocation);
		std::swap(m_extent, other.m_extent);
		std::swap(m_mipLevels, other.m_mipLevels);
		std::swap(m_format, other.m_format);
		std::swap(m_usageFlags, other.m_usageFlags);
		std::swap(m_type, other.m_type);
		std::swap(m_layout, other.m_layout);
	}

	Texture::~Texture()
	{
		const bool was_allocated = m_image && m_allocation;
		if (was_allocated)
		{
			auto allocator = m_device->get_allocator();
			allocator.destroyImage(m_image, m_allocation);
		}
	}

	auto Texture::operator=(Texture&& rhs) noexcept -> Texture&
	{
		std::swap(m_device, rhs.m_device);
		std::swap(m_image, rhs.m_image);
		std::swap(m_allocation, rhs.m_allocation);
		std::swap(m_extent, rhs.m_extent);
		std::swap(m_mipLevels, rhs.m_mipLevels);
		std::swap(m_format, rhs.m_format);
		std::swap(m_usageFlags, rhs.m_usageFlags);
		std::swap(m_type, rhs.m_type);
		std::swap(m_layout, rhs.m_layout);
		return *this;
	}

	Pipeline::Pipeline(PipelineType pipelineType, const std::vector<vk::DescriptorSetLayout>& setLayouts)
		: m_pipelineType(pipelineType), m_setLayouts(setLayouts)
	{
	}

	Pipeline::Pipeline(Pipeline&& other) noexcept
	{
		std::swap(m_pipelineType, other.m_pipelineType);
		std::swap(m_pipeline, other.m_pipeline);
	}

	auto Pipeline::operator=(Pipeline&& rhs) noexcept -> Pipeline&
	{
		std::swap(m_pipelineType, rhs.m_pipelineType);
		std::swap(m_pipeline, rhs.m_pipeline);
		return *this;
	}

	ComputePipeline::ComputePipeline(vk::Device device, const std::vector<char>& shaderCode, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, vk::PushConstantRange constantRange)
		: Pipeline(PipelineType::eCompute, descriptorSetLayouts)
	{
		vk::PipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.setSetLayouts(get_set_layouts());
		if (constantRange.size > 0 && constantRange.stageFlags != vk::ShaderStageFlags())
		{
			pipeline_layout_info.setPushConstantRanges(constantRange);
		}
		m_layout = device.createPipelineLayoutUnique(pipeline_layout_info);

		vk::ShaderModuleCreateInfo module_info{};
		module_info.setCodeSize(shaderCode.size());
		module_info.setPCode(reinterpret_cast<const std::uint32_t*>(shaderCode.data()));
		auto module = device.createShaderModuleUnique(module_info);

		vk::PipelineShaderStageCreateInfo stage_info{};
		stage_info.setStage(vk::ShaderStageFlagBits::eCompute);
		stage_info.setModule(module.get());
		stage_info.setPName("Main");

		vk::ComputePipelineCreateInfo vk_pipeline_info{};
		vk_pipeline_info.setStage(stage_info);
		vk_pipeline_info.setLayout(m_layout.get());

		m_pipeline = device.createComputePipelineUnique({}, vk_pipeline_info).value;
	}

	GraphicsPipeline::GraphicsPipeline(vk::Device device, const std::vector<char>& vertexCode, const std::vector<char>& fragmentCode, const std::vector<VertexAttribute>& vertexAttributes, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, vk::PushConstantRange constantRange)
		: Pipeline(PipelineType::eGraphics, descriptorSetLayouts)
	{
		vk::PipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.setSetLayouts(get_set_layouts());
		if (constantRange.size > 0 && constantRange.stageFlags != vk::ShaderStageFlags())
		{
			pipeline_layout_info.setPushConstantRanges(constantRange);
		}
		m_layout = device.createPipelineLayoutUnique(pipeline_layout_info);

		vk::ShaderModuleCreateInfo vertex_module_info{};
		vertex_module_info.setCodeSize(vertexCode.size());
		vertex_module_info.setPCode(reinterpret_cast<const std::uint32_t*>(vertexCode.data()));
		auto vertex_module = device.createShaderModuleUnique(vertex_module_info);
		vk::PipelineShaderStageCreateInfo vertex_stage_info{};
		vertex_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
		vertex_stage_info.setModule(vertex_module.get());
		vertex_stage_info.setPName("vs_main");

		vk::ShaderModuleCreateInfo fragment_module_info{};
		fragment_module_info.setCodeSize(fragmentCode.size());
		fragment_module_info.setPCode(reinterpret_cast<const std::uint32_t*>(fragmentCode.data()));
		auto fragment_module = device.createShaderModuleUnique(fragment_module_info);
		vk::PipelineShaderStageCreateInfo fragment_stage_info{};
		fragment_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
		fragment_stage_info.setModule(fragment_module.get());
		fragment_stage_info.setPName("ps_main");

		const std::vector stages = { vertex_stage_info, fragment_stage_info };

		std::uint32_t stride{};
		std::vector<vk::VertexInputAttributeDescription> vk_attributes(vertexAttributes.size());
		for (auto i = 0; i < vk_attributes.size(); ++i)
		{
			const auto& attribute = vertexAttributes.at(i);
			auto& vk_attribute = vk_attributes.at(i);
			vk_attribute.setBinding(i);
			vk_attribute.setFormat(convert_format_to_vk_format(attribute.format));
			vk_attribute.setOffset(stride);

			stride += convert_format_to_byte_size(attribute.format);
		}

		vk::VertexInputBindingDescription vk_binding{};
		vk_binding.setBinding(0);
		vk_binding.setInputRate(vk::VertexInputRate::eVertex);
		vk_binding.setStride(stride);

		vk::PipelineVertexInputStateCreateInfo vertex_input_state{};
		if (!vertexAttributes.empty())
		{
			vertex_input_state.setVertexAttributeDescriptions(vk_attributes);
			vertex_input_state.setVertexBindingDescriptions(vk_binding);
		}

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{};
		input_assembly_state.setTopology(vk::PrimitiveTopology::eTriangleList);

		vk::PipelineViewportStateCreateInfo viewport_state{};
		viewport_state.setViewportCount(1);
		viewport_state.setScissorCount(1);

		vk::PipelineRasterizationStateCreateInfo rasterisation_state{};
		rasterisation_state.setPolygonMode(vk::PolygonMode::eFill);	  // TODO: Optional.
		rasterisation_state.setCullMode(vk::CullModeFlagBits::eNone); // TODO: Optional.
		rasterisation_state.setFrontFace(vk::FrontFace::eClockwise);  // TODO: Optional.
		rasterisation_state.setLineWidth(1.0f);						  // TODO: Optional.

		vk::PipelineMultisampleStateCreateInfo multisample_state{};
		multisample_state.setRasterizationSamples(vk::SampleCountFlagBits::e1); // #TODO: Optional.

		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{};
		depth_stencil_state.setDepthTestEnable(false);	 // #TODO: Optional.
		depth_stencil_state.setDepthWriteEnable(false);	 // #TODO: Optional.
		depth_stencil_state.setStencilTestEnable(false); // #TODO: Optional.

		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments{
			vk::PipelineColorBlendAttachmentState(
				VK_FALSE,
				vk::BlendFactor::eZero,
				vk::BlendFactor::eOne,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eZero,
				vk::BlendFactor::eZero,
				vk::BlendOp::eAdd,
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		};
		vk::PipelineColorBlendStateCreateInfo color_blend_state{};
		color_blend_state.setAttachments(colorBlendAttachments);

		std::vector<vk::DynamicState> dynamicStates{
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};
		vk::PipelineDynamicStateCreateInfo dynamic_state{};
		dynamic_state.setDynamicStates(dynamicStates);

		std::vector<vk::Format> colorAttachmentFormats{ vk::Format::eB8G8R8A8Srgb }; // #TODO: Optional.
		vk::PipelineRenderingCreateInfo rendering_info{};
		rendering_info.setColorAttachmentFormats(colorAttachmentFormats);
		rendering_info.setDepthAttachmentFormat({}); // #TODO: Optional.

		vk::GraphicsPipelineCreateInfo vk_pipeline_info{};
		vk_pipeline_info.setStages(stages);
		vk_pipeline_info.setLayout(m_layout.get());
		vk_pipeline_info.setPVertexInputState(&vertex_input_state);
		vk_pipeline_info.setPInputAssemblyState(&input_assembly_state);
		vk_pipeline_info.setPViewportState(&viewport_state);
		vk_pipeline_info.setPRasterizationState(&rasterisation_state);
		vk_pipeline_info.setPMultisampleState(&multisample_state);
		vk_pipeline_info.setPDepthStencilState(&depth_stencil_state);
		vk_pipeline_info.setPColorBlendState(&color_blend_state);
		vk_pipeline_info.setPDynamicState(&dynamic_state);
		vk_pipeline_info.setPNext(&rendering_info);

		m_pipeline = device.createGraphicsPipelineUnique({}, vk_pipeline_info).value;
	}

	SwapChain::SwapChain(Device& device, const SwapChainInfo& swapChainInfo)
		: m_device(&device)
	{
		auto* context = m_device->get_context();
		GFX_ASSERT(context, "context should not be nullptr!");
		auto vk_instance = context->get_instance();
		GFX_ASSERT(vk_instance, "vk_instance should be valid!");

#if _WIN32
		HINSTANCE hinstance = GetModuleHandle(nullptr);
		HWND hwnd = static_cast<HWND>(swapChainInfo.platformWindowHandle);
		vk::Win32SurfaceCreateInfoKHR surface_info{};
		surface_info.setHinstance(hinstance);
		surface_info.setHwnd(hwnd);
		m_surface = vk_instance.createWin32SurfaceKHRUnique(surface_info);
#endif

		m_fence = m_device->get_device().createFenceUnique({});

		resize(swapChainInfo.initialWidth, swapChainInfo.initialHeight);

		acquire_next_image_index();
	}

	SwapChain::SwapChain(SwapChain&& other) noexcept
	{
		std::swap(m_device, other.m_device);
		std::swap(m_surface, other.m_surface);
		std::swap(m_swapChain, other.m_swapChain);
	}

	SwapChain::~SwapChain()
	{
		cleanup();
	}

	void SwapChain::resize(std::int32_t width, std::uint32_t height)
	{
		cleanup();

		auto surfaceCapabilities = m_device->get_physical_device().getSurfaceCapabilitiesKHR(m_surface.get());

		std::uint32_t minImageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount != -1 && minImageCount > surfaceCapabilities.maxImageCount)
		{
			minImageCount = surfaceCapabilities.maxImageCount;
		}

		const std::vector preferredSurfaceFormats{
			vk::Format::eB8G8R8A8Srgb, vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm
		};
		auto surfaceFormat = vk::SurfaceFormatKHR(m_device->get_first_supported_surface_format(preferredSurfaceFormats, m_surface.get()), vk::ColorSpaceKHR::eSrgbNonlinear);

		m_extent = vk::Extent2D(width, height);
		m_extent.width = std::clamp(m_extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		m_extent.height = std::clamp(m_extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

		auto presentMode = vk::PresentModeKHR::eFifo; // VSync. (FIFI is required to be supported.)
		const auto preferredPresentMode = m_vsyncEnabled ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eImmediate;
		if (m_device->is_present_mode_supported(preferredPresentMode, m_surface.get()))
		{
			presentMode = preferredPresentMode;
		}

		auto oldSwapChain = std::move(m_swapChain);

		vk::SwapchainCreateInfoKHR swap_chain_info{};
		swap_chain_info.setSurface(m_surface.get());
		swap_chain_info.setMinImageCount(minImageCount);
		swap_chain_info.setImageFormat(surfaceFormat.format);
		swap_chain_info.setImageColorSpace(surfaceFormat.colorSpace);
		swap_chain_info.setImageExtent(m_extent);
		swap_chain_info.setImageArrayLayers(1);
		swap_chain_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
		swap_chain_info.setPresentMode(presentMode);
		swap_chain_info.setOldSwapchain(oldSwapChain.get());

		auto vk_device = m_device->get_device();
		m_swapChain = vk_device.createSwapchainKHRUnique(swap_chain_info);

		auto images = vk_device.getSwapchainImagesKHR(m_swapChain.get());
		m_imageHandles.resize(images.size());
		for (auto i = 0; i < images.size(); ++i)
		{
			bool success = m_device->create_texture(m_imageHandles[i], images[i], vk::Extent3D(m_extent, 1), surfaceFormat.format);
			GFX_ASSERT(success, "Failed to create Texture from SwapChain image!");
		}
	}

	void SwapChain::present(vk::Queue queue, vk::Semaphore waitSemaphore)
	{
		vk::PresentInfoKHR present_info{};
		present_info.setSwapchains(m_swapChain.get());
		present_info.setImageIndices(m_imageIndex);
		if (waitSemaphore)
		{
			present_info.setWaitSemaphores(waitSemaphore);
		}
		auto result = queue.presentKHR(present_info);
		GFX_UNUSED(result);

		acquire_next_image_index();
	}

	auto SwapChain::operator=(SwapChain&& rhs) noexcept -> SwapChain&
	{
		std::swap(m_device, rhs.m_device);
		std::swap(m_surface, rhs.m_surface);
		std::swap(m_swapChain, rhs.m_swapChain);
		return *this;
	}

	void SwapChain::cleanup()
	{
		for (auto imageHandle : m_imageHandles)
		{
			m_device->destroy_texture(imageHandle);
		}
		m_imageHandles.clear();
	}

	void SwapChain::acquire_next_image_index()
	{
		auto device = m_device->get_device();
		m_imageIndex = device.acquireNextImageKHR(m_swapChain.get(), std::uint64_t(-1), {}, m_fence.get()).value;
		auto result = device.waitForFences(m_fence.get(), VK_TRUE, std::uint64_t(-1));
		GFX_UNUSED(result);
		device.resetFences(m_fence.get());
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data)
	{
		s_errorCallback(callback_data->pMessage);

		return VK_FALSE;
	}

#pragma endregion

} // namespace sm::gfx