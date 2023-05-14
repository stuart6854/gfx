/*
 * Copyright (c) Stuart Millman 2023.
 */

#ifndef GFX_GFX_P_HPP
#define GFX_GFX_P_HPP

#include "gfx.hpp"

#if _WIN32
	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#define VMA_IMPLEMENTATION
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

#include <memory>
#include <unordered_map>

namespace sm::gfx
{
	VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data);

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
	class Pipeline;
	class Buffer;
	class SwapChain;

	class Device
	{
	public:
		Device() = default;
		Device(Context& context, DeviceHandle deviceHandle, const DeviceInfo& deviceInfo);
		~Device() = default;
		DISABLE_COPY_AND_MOVE(Device);

		bool is_valid() const;

		auto get_context() -> Context* { return m_context; }
		auto get_physical_device() const -> vk::PhysicalDevice { return m_physicalDevice; }
		auto get_device() const -> vk::Device { return m_device.get(); }
		bool get_queue(vk::Queue& outQueue, std::uint32_t queueIndex);

		bool is_present_mode_supported(vk::PresentModeKHR presentMode, vk::SurfaceKHR surface) const;
		auto get_first_supported_surface_format(const std::vector<vk::Format>& formats, vk::SurfaceKHR surface) -> vk::Format;

		void wait_on_fence(FenceHandle fenceHandle);

		auto create_command_list(CommandListHandle& outCommandListHandle, std::uint32_t queueIndex) -> bool;
		bool get_command_list(CommandList*& outCommandList, CommandListHandle commandListHandle);
		bool submit_command_list(const SubmitInfo& submitInfo, FenceHandle* outFenceHandle, SemaphoreHandle* outSemaphoreHandle);

		bool create_or_get_descriptor_set_layout(vk::DescriptorSetLayout& outDescriptorSetLayout, const DescriptorSetInfo& descriptorSetInfo);

		bool create_compute_pipeline(PipelineHandle& outPipelineHandle, const ComputePipelineInfo& computePipelineInfo);
		bool create_graphics_pipeline(PipelineHandle& outPipelineHandle, const GraphicsPipelineInfo& graphicsPipelineInfo);
		void destroy_pipeline(PipelineHandle pipelineHandle);
		bool get_pipeline(Pipeline*& outPipeline, PipelineHandle pipelineHandle);

		bool create_descriptor_set_from_pipeline(DescriptorSetHandle& outDescriptorSetHandle, PipelineHandle pipelineHandle, std::uint32_t set);
		bool get_descriptor_set(vk::DescriptorSet& outDescriptorSet, DescriptorSetHandle descriptorSetHandle);
		void bind_buffer_to_descriptor_set(DescriptorSetHandle descriptorSetHandle, std::uint32_t binding, BufferHandle bufferHandle);

		bool create_buffer(BufferHandle& outBufferHandle, const BufferInfo& bufferInfo);
		void destroy_buffer(BufferHandle bufferHandle);
		bool map_buffer(BufferHandle bufferHandle, void*& outBufferPtr);
		void unmap_buffer(BufferHandle bufferHandle);

		bool create_swap_chain(SwapChainHandle& outSwapChainHandle, const SwapChainInfo& swapChainInfo);
		void destroy_swap_chain(SwapChainHandle swapChainHandle);

	private:
		auto create_fence() -> FenceHandle;

		static auto get_descriptor_set_layout_binding(const DescriptorBindingInfo& descriptorBindingInfo) -> vk::DescriptorSetLayoutBinding;

	private:
		Context* m_context{ nullptr };
		DeviceHandle m_deviceHandle;

		vk::PhysicalDevice m_physicalDevice;
		vk::UniqueDevice m_device;
		vma::UniqueAllocator m_allocator;

		std::vector<std::uint32_t> m_queueFlags;
		std::vector<std::uint32_t> m_queueFamilies;
		std::vector<vk::Queue> m_queues;
		std::unordered_map<std::uint32_t, vk::UniqueCommandPool> m_queueFamilyCommandPoolMap;

		vk::UniqueDescriptorPool m_descriptorPool;

		std::unordered_map<ResourceHandle, vk::UniqueFence> m_fenceMap;
		std::uint32_t m_nextFenceId{ 1 };

		std::unordered_map<ResourceHandle, std::unique_ptr<CommandList>> m_commandListMap;
		std::uint32_t m_nextCommandListId{ 1 };

		std::unordered_map<std::size_t, vk::UniqueDescriptorSetLayout> m_descriptorSetLayoutMap;

		std::unordered_map<ResourceHandle, std::unique_ptr<Pipeline>> m_pipelineMap;
		std::uint32_t m_nextPipelineId{ 1 };

		std::unordered_map<ResourceHandle, vk::UniqueDescriptorSet> m_descriptorSetMap;
		std::uint32_t m_nextDescriptorSetId{ 1 };

		std::unordered_map<ResourceHandle, std::unique_ptr<Buffer>> m_bufferMap;
		std::uint32_t m_nextBufferId{ 1 };

		std::unordered_map<ResourceHandle, std::unique_ptr<SwapChain>> m_swapChainMap;
		std::uint32_t m_nextSwapChainId{ 1 };
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

		void bind_pipeline(Pipeline* pipeline);
		void bind_descriptor_set(vk::DescriptorSet descriptorSet);

		void dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ);

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
		Pipeline* m_boundPipeline{ nullptr };
	};

	enum class PipelineType
	{
		eCompute,
		eGraphics
	};

	class Pipeline
	{
	public:
		Pipeline() = default;
		explicit Pipeline(PipelineType pipelineType, const std::vector<vk::DescriptorSetLayout>& setLayouts);
		Pipeline(Pipeline&& other) noexcept;
		virtual ~Pipeline() = default;

		GFX_DISABLE_COPY(Pipeline);

		/* Getters */

		auto get_set_layouts() const -> const std::vector<vk::DescriptorSetLayout>& { return m_setLayouts; }
		auto get_set_layout(std::uint32_t set) const -> vk::DescriptorSetLayout { return m_setLayouts.at(set); }
		auto get_pipeline_layout() const -> vk::PipelineLayout { return m_layout.get(); }
		auto get_pipeline() const -> vk::Pipeline { return m_pipeline.get(); }
		auto get_type() const -> PipelineType { return m_pipelineType; }

		/* Operators */

		auto operator=(Pipeline&& rhs) noexcept -> Pipeline&;

	protected:
		vk::UniquePipelineLayout m_layout;
		vk::UniquePipeline m_pipeline;

	private:
		PipelineType m_pipelineType{};
		std::vector<vk::DescriptorSetLayout> m_setLayouts;
	};

	class ComputePipeline final : public Pipeline
	{
	public:
		ComputePipeline() = default;
		ComputePipeline(vk::Device device, const std::vector<char>& shaderCode, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
		~ComputePipeline() override = default;

	private:
	};

	class GraphicsPipeline final : public Pipeline
	{
	public:
		GraphicsPipeline() = default;
		GraphicsPipeline(vk::Device device, const std::vector<char>& vertexCode, const std::vector<char>& fragmentCode, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
		~GraphicsPipeline() override = default;

	private:
		vk::UniqueDescriptorSetLayout m_setLayout;
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

		auto get_descriptor_type() const -> auto { return m_descriptorType; }
		auto get_descriptor_info() const -> const vk::DescriptorBufferInfo& { return m_descriptorInfo; }

		/* Operators */

		auto operator=(Buffer&& rhs) noexcept -> Buffer&;

	private:
		vk::Device m_device;
		vma::Allocator m_allocator;

		vma::UniqueBuffer m_buffer;
		vma::UniqueAllocation m_allocation;

		vk::DescriptorType m_descriptorType;
		vk::DescriptorBufferInfo m_descriptorInfo;
	};

	class SwapChain
	{
	public:
		SwapChain() = default;
		explicit SwapChain(Device& device, const SwapChainInfo& swapChainInfo);
		SwapChain(SwapChain&& other) noexcept;
		~SwapChain() = default;

		GFX_DISABLE_COPY(SwapChain);

		void resize(std::int32_t width, std::uint32_t height);

		/* Getters */

		auto get_surface() const -> vk::SurfaceKHR { return m_surface.get(); }
		auto get_swap_chain() const -> vk::SwapchainKHR { return m_swapChain.get(); }

		/* Operators */

		auto operator=(SwapChain&& rhs) noexcept -> SwapChain&;

	private:
		Device* m_device{ nullptr };

		vk::UniqueSurfaceKHR m_surface;
		vk::UniqueSwapchainKHR m_swapChain;

		vk::Extent2D m_extent;
		bool m_vsyncEnabled{};
	};

} // namespace sm::gfx

#endif // GFX_GFX_P_HPP
