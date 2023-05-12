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

	void destroy_compute_pipeline(PipelineHandle pipelineHandle)
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

#pragma endregion

#pragma endregion

#pragma region Private Header

	VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data);

	Context::Context(const AppInfo& appInfo)
	{
		auto vkGetInstanceProcAddr = m_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

		vk::ApplicationInfo vk_app_info{};
		vk_app_info.setApiVersion(VK_API_VERSION_1_3);
		vk_app_info.setPApplicationName(appInfo.appName.c_str());

		std::vector<const char*> extensions = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
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

		auto device = std::make_unique<Device>(deviceHandle, s_context->get_instance(), deviceInfo);
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

	Device::Device(DeviceHandle deviceHandle, vk::Instance instance, const DeviceInfo& deviceInfo)
		: m_deviceHandle(deviceHandle)
	{
		auto physicalDevices = instance.enumeratePhysicalDevices();
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

		vk::DeviceCreateInfo vk_device_info{};
		vk_device_info.setQueueCreateInfos(queue_info_vec);
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
		allocator_info.setInstance(instance);
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
		m_descriptorPool = m_device->createDescriptorPoolUnique(descriptor_pool_info);
	}

	bool Device::is_valid() const
	{
		return static_cast<bool>(*m_device);
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

		m_pipelineMap[pipelineHandle.resourceHandle] = std::make_unique<ComputePipeline>(m_device.get(), computePipelineInfo.shaderCode, setLayouts);
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

	void CommandList::bind_pipeline(Pipeline* pipeline)
	{
		if (pipeline == nullptr)
		{
			s_errorCallback("GFX - Command list cannot bind null pipeline!");
			return;
		}

		const vk::PipelineBindPoint bindPoint = pipeline->get_type() == PipelineType::eCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics;
		m_commandBuffer->bindPipeline(bindPoint, pipeline->get_pipeline());
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
		vk_buffer_info.setUsage(vk::BufferUsageFlagBits::eStorageBuffer); // #TODO: Make optional.
		vk_buffer_info.setSize(bufferInfo.size);
		vk_buffer_info.setSharingMode(vk::SharingMode::eExclusive);
		//		vk_buffer_info.setQueueFamilyIndices(); // #TODO: Add later?

		vma::AllocationCreateInfo alloc_info{};
		alloc_info.setUsage(vma::MemoryUsage::eAutoPreferDevice);						// #TODO: Make optional.
		alloc_info.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite); // #TODO: Optional.

		std::tie(m_buffer, m_allocation) = m_allocator.createBufferUnique(vk_buffer_info, alloc_info);
	}

	Buffer::Buffer(Buffer&& other) noexcept
	{
		std::swap(m_device, other.m_device);
		std::swap(m_allocation, other.m_allocation);
		std::swap(m_buffer, other.m_buffer);
		std::swap(m_allocation, other.m_allocation);
	}

	auto Buffer::operator=(Buffer&& rhs) noexcept -> Buffer&
	{
		std::swap(m_device, rhs.m_device);
		std::swap(m_allocation, rhs.m_allocation);
		std::swap(m_buffer, rhs.m_buffer);
		std::swap(m_allocation, rhs.m_allocation);
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

	ComputePipeline::ComputePipeline(vk::Device device, const std::vector<char>& shaderCode, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
		: Pipeline(PipelineType::eCompute, descriptorSetLayouts)
	{
		std::vector<vk::DescriptorSetLayoutBinding> set_layout_bindings{
			{ 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
			{ 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
		};
		vk::DescriptorSetLayoutCreateInfo set_layout_info{};
		set_layout_info.setBindings(set_layout_bindings);
		m_setLayout = device.createDescriptorSetLayoutUnique(set_layout_info);

		vk::PipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.setSetLayouts(m_setLayout.get());
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