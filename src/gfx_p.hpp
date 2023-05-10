/*
 * Copyright (c) Stuart Millman 2023.
 */

#ifndef GFX_GFX_P_HPP
#define GFX_GFX_P_HPP

#include "gfx.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <memory>
#include <unordered_map>

namespace sm::gfx
{
	class Device;

	class Context
	{
	public:
		Context() = default;
		explicit Context(const AppInfo& appInfo);
		~Context() = default;
		DISABLE_COPY_AND_MOVE(Context);

		bool is_valid() const;

		auto create_device(DeviceHandle& outDeviceHandle, const DeviceInfo& deviceInfo) -> bool;
		void destroy_device(DeviceHandle deviceHandle);
		bool get_device(Device*& outDevice, DeviceHandle deviceHandle);

		/* Getters */

		auto get_instance() const -> vk::Instance { return m_instance.get(); }

	private:
		vk::DynamicLoader m_loader;
		vk::UniqueInstance m_instance;
		vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;

		std::unordered_map<DeviceHandle, std::unique_ptr<Device>> m_deviceMap;
		std::uint32_t m_nextDeviceId{ 1 };
	};

	class CommandList;

	class Device
	{
	public:
		Device() = default;
		Device(vk::Instance instance, const DeviceInfo& deviceInfo);
		~Device() = default;
		DISABLE_COPY_AND_MOVE(Device);

		bool is_valid() const;

		auto create_command_list(CommandListHandle& outCommandListHandle, std::uint32_t queueFlags) -> bool;
		bool get_command_list(CommandList*& outCommandList, CommandListHandle commandListHandle);
		bool submit_command_list(CommandListHandle commandListHandle, FenceHandle* outFenceHandle, SemaphoreHandle* outSemaphoreHandle);

	private:
		vk::PhysicalDevice m_physicalDevice;
		vk::UniqueDevice m_device;

		std::unordered_map<std::uint32_t, std::uint32_t> m_queueFlagsQueueFamilyMap;
		std::unordered_map<std::uint32_t, vk::UniqueCommandPool> m_queueFamilyCommandPoolMap;
		std::unordered_map<std::uint32_t, vk::Queue> m_queueFlagsQueueMap;

		std::unordered_map<ResourceHandle, std::unique_ptr<CommandList>> m_commandListMap;
		std::uint32_t m_nextCommandListId{ 1 };
	};

	class CommandList
	{
	public:
		CommandList() = default;
		explicit CommandList(vk::Device device, vk::CommandPool commandPool, vk::Queue queue);
		CommandList(CommandList&& other) noexcept;
		~CommandList() = default;

		GFX_DISABLE_COPY(CommandList);

		bool is_valid() const;

		void reset();

		void begin();
		void end();

		void draw(std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex, std::uint32_t first_instance);
		void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance);

		/* Getters */

		auto get_queue() const -> vk::Queue { return m_queue; }
		auto get_command_buffer() const -> vk::CommandBuffer { return m_commandBuffer.get(); }

		/* Operators */

		auto operator=(CommandList&& rhs) noexcept -> CommandList&;

	private:
		vk::Device m_device;
		vk::CommandPool m_commandPool;
		vk::Queue m_queue;

		vk::UniqueCommandBuffer m_commandBuffer;

		bool m_hasBegun{ false };
	};

} // namespace sm::gfx

#endif // GFX_GFX_P_HPP
