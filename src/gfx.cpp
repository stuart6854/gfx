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

	bool create_command_list(CommandListHandle& outCommandListHandle, DeviceHandle deviceHandle, std::uint32_t queueFlags)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return device->create_command_list(outCommandListHandle, queueFlags);
	}

	void destroy_command_list(DeviceHandle deviceHandle, CommandListHandle commandListHandle)
	{
	}

	void submit_command_list(CommandListHandle commandListHandle, FenceHandle* OutFenceHandle, SemaphoreHandle* OutSemaphoreHandle)
	{
	}

#pragma region Command List Recording

	bool begin(CommandListHandle commandListHandle)
	{
		GFX_ASSERT(s_context && s_context->is_valid(), "GFX has not been initialised!");

		Device* device{ nullptr };
		if (!s_context->get_device(device, commandListHandle.deviceHandle))
		{
			return false;
		}
		GFX_ASSERT(device != nullptr, "Device should not be null!");

		return false;
	}

	void end(CommandListHandle commandListHandle)
	{
	}

	void draw(CommandListHandle commandListHandle, std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex, std::uint32_t first_instance)
	{
	}
	void draw_indexed(CommandListHandle commandListHandle, std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance)
	{
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

			if ((deviceInfo.deviceFlags & DeviceFlags_PreferIntegrated) == 1 && deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			{
				score += 1000;
			}
			if ((deviceInfo.deviceFlags & DeviceFlags_PreferDiscrete) == 1 && deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
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

		std::unordered_map<std::uint32_t, std::uint32_t> usedQueueFamilyCounts;
		for (auto queue_flags : deviceInfo.queueFlags)
		{
			vk::QueueFlags wanted_flags;
			if (queue_flags & QueueFlags_Graphics)
			{
				wanted_flags |= vk::QueueFlagBits::eGraphics;
			}
			if (queue_flags & QueueFlags_Compute)
			{
				wanted_flags |= vk::QueueFlagBits::eCompute;
			}
			if (queue_flags & QueueFlags_Transfer)
			{
				wanted_flags |= vk::QueueFlagBits::eTransfer;
			}

			for (auto i = 0; i < queueProperties.size(); ++i)
			{
				auto& queueProps = queueProperties[i];
				if (queueProps.queueFlags & wanted_flags)
				{
					usedQueueFamilyCounts[i] += 1;
					m_queueFlagsQueueFamilyMap[queue_flags] = i;
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
		for (auto [queueFlags, queueFamily] : m_queueFlagsQueueFamilyMap)
		{
			vk::CommandPoolCreateInfo cmd_pool_info{};
			cmd_pool_info.setQueueFamilyIndex(queueFamily);
			cmd_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
			m_queueFamilyCommandPoolMap[queueFamily] = m_device->createCommandPoolUnique(cmd_pool_info);

			auto queueIndex = queueIndexMap[queueFamily];
			m_queueFlagsQueueMap[queueFlags] = m_device->getQueue(queueFamily, queueIndex);
			queueIndexMap[queueFamily] += 1;
		}
	}

	bool Device::is_valid() const
	{
		return static_cast<bool>(*m_device);
	}

	auto Device::create_command_list(CommandListHandle& outCommandListHandle, std::uint32_t queueFlags) -> bool
	{
		auto queueFamily = m_queueFlagsQueueFamilyMap.at(queueFlags);
		auto commandPool = m_queueFamilyCommandPoolMap.at(queueFamily).get();

		CommandListHandle commandListHandle(m_deviceHandle, ResourceHandle(m_nextCommandListId));

		m_commandListMap[commandListHandle.resourceHandle] = std::make_unique<CommandList>(m_device.get(), commandPool, vk::Queue());
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

	bool Device::submit_command_list(CommandListHandle commandListHandle, FenceHandle* outFenceHandle, SemaphoreHandle* outSemaphoreHandle)
	{
		if (!m_commandListMap.contains(commandListHandle.resourceHandle))
		{
			return false;
		}

		vk::Fence fence{};
		if (outFenceHandle != nullptr)
		{
			// #TODO: Create fence
		}

		vk::Semaphore signalSemaphore{};
		if (outSemaphoreHandle != nullptr)
		{
			// TODO: Create signalSemaphore
		}

		const auto& command_list = m_commandListMap.at(commandListHandle.resourceHandle);
		auto command_buffer = command_list->get_command_buffer();

		vk::SubmitInfo submit_info{};
		submit_info.setCommandBuffers(command_buffer);
		submit_info.setSignalSemaphores(signalSemaphore);

		auto queue = command_list->get_queue();
		queue.submit(submit_info, fence);

		return true;
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
		m_commandBuffer.reset();
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
		return *this;
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