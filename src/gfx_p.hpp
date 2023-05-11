/*
 * Copyright (c) Stuart Millman 2023.
 */

#ifndef GFX_GFX_P_HPP
#define GFX_GFX_P_HPP

#include "gfx.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

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
	class Buffer;

	class Device
	{
	public:
		Device() = default;
		Device(DeviceHandle deviceHandle, vk::Instance instance, const DeviceInfo& deviceInfo);
		~Device() = default;
		DISABLE_COPY_AND_MOVE(Device);

		bool is_valid() const;

		void wait_on_fence(FenceHandle fenceHandle);

		auto create_command_list(CommandListHandle& outCommandListHandle, std::uint32_t queueIndex) -> bool;
		bool get_command_list(CommandList*& outCommandList, CommandListHandle commandListHandle);
		bool submit_command_list(const SubmitInfo& submitInfo, FenceHandle* outFenceHandle, SemaphoreHandle* outSemaphoreHandle);

		bool create_buffer(BufferHandle& outBufferHandle, const BufferInfo& bufferInfo);
		void destroy_buffer(BufferHandle bufferHandle);
		bool map_buffer(BufferHandle bufferHandle, void*& outBufferPtr);
		void unmap_buffer(BufferHandle bufferHandle);

	private:
		auto create_fence() -> FenceHandle;

	private:
		DeviceHandle m_deviceHandle;

		vk::PhysicalDevice m_physicalDevice;
		vk::UniqueDevice m_device;
		vma::UniqueAllocator m_allocator;

		std::vector<std::uint32_t> m_queueFlags;
		std::vector<std::uint32_t> m_queueFamilies;
		std::vector<vk::Queue> m_queues;
		std::unordered_map<std::uint32_t, vk::UniqueCommandPool> m_queueFamilyCommandPoolMap;

		std::unordered_map<ResourceHandle, vk::UniqueFence> m_fenceMap;
		std::uint32_t m_nextFenceId{ 1 };

		std::unordered_map<ResourceHandle, std::unique_ptr<CommandList>> m_commandListMap;
		std::uint32_t m_nextCommandListId{ 1 };

		std::unordered_map<ResourceHandle, std::unique_ptr<Buffer>> m_bufferMap;
		std::uint32_t m_nextBufferId{ 1 };
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

	class Buffer
	{
	public:
		Buffer() = default;
		explicit Buffer(vk::Device device, vma::Allocator allocator, const BufferInfo& bufferInfo);
		Buffer(Buffer&& other) noexcept;
		~Buffer() = default;

		GFX_DISABLE_COPY(Buffer);

		/* Getters */

		auto get_buffer() const -> vk::Buffer { return m_buffer.get(); }
		auto get_allocation() const -> vma::Allocation { return m_allocation.get(); }

		/* Operators */

		auto operator=(Buffer&& rhs) noexcept -> Buffer&;

	private:
		vk::Device m_device;
		vma::Allocator m_allocator;

		vma::UniqueBuffer m_buffer;
		vma::UniqueAllocation m_allocation;
	};

} // namespace sm::gfx

#endif // GFX_GFX_P_HPP
