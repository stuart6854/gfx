/*
 * Copyright (c) Stuart Millman 2023.
 */

#ifndef GFX_GFX_HPP
#define GFX_GFX_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

#ifndef GFX_UNUSED
	#define GFX_UNUSED(_x) (void)(_x)
#endif

#ifndef GFX_LOG_ERR
	#include <iostream>
	#include <format>
	#define GFX_LOG_ERR(_msg) std::cerr << _msg << std::endl
	#define GFX_LOG_ERR_FMT(_fmt, ...) std::cerr << std::format(_fmt, __VA_ARGS__) << std::endl
#endif

#ifndef GFX_ASSERT
	#define GFX_ASSERT(_expr, _msg) \
		do                          \
		{                           \
			if (!(_expr))           \
			{                       \
				GFX_LOG_ERR(_msg);  \
				__debugbreak();     \
			}                       \
		}                           \
		while (false)
#endif

#define GFX_DISABLE_COPY(_className)        \
	_className(const _className&) = delete; \
	auto operator=(const _className&)->_className& = delete
#define GFX_DISABLE_MOVE(_className)   \
	_className(_className&&) = delete; \
	auto operator=(_className&&)->_className& = delete

#define DISABLE_COPY_AND_MOVE(_className) \
	GFX_DISABLE_COPY(_className);         \
	GFX_DISABLE_MOVE(_className)

#define GFX_DEFINE_HANDLE(_typename)     \
	enum class _typename : std::uint32_t \
	{                                    \
	}

#define GFX_DEFINE_RESOURCE_HANDLE(_typename)                                                                                               \
	struct _typename                                                                                                                        \
	{                                                                                                                                       \
		union                                                                                                                               \
		{                                                                                                                                   \
			std::uint64_t fullHandle{};                                                                                                     \
			struct                                                                                                                          \
			{                                                                                                                               \
				DeviceHandle deviceHandle;                                                                                                  \
				ResourceHandle resourceHandle;                                                                                              \
			};                                                                                                                              \
		};                                                                                                                                  \
		_typename() = default;                                                                                                              \
		_typename(DeviceHandle deviceHandle, ResourceHandle resourceHandle) : deviceHandle(deviceHandle), resourceHandle(resourceHandle) {} \
	}

#define CAST_HANDLE_TO_INT(_handle) static_cast<std::uint32_t>(_handle)

namespace sm
{
	template <class T>
	inline void hash_combine(std::size_t& seed, const T& value)
	{
		std::hash<T> hasher{};
		seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

} // namespace sm

namespace sm::gfx
{
	GFX_DEFINE_HANDLE(DeviceHandle);
	GFX_DEFINE_HANDLE(ResourceHandle);
	GFX_DEFINE_RESOURCE_HANDLE(CommandListHandle);
	GFX_DEFINE_RESOURCE_HANDLE(FenceHandle);
	GFX_DEFINE_RESOURCE_HANDLE(SemaphoreHandle);
	GFX_DEFINE_RESOURCE_HANDLE(PipelineHandle);
	GFX_DEFINE_RESOURCE_HANDLE(DescriptorSetHandle);
	GFX_DEFINE_RESOURCE_HANDLE(BufferHandle);
	GFX_DEFINE_RESOURCE_HANDLE(TextureHandle);
	GFX_DEFINE_RESOURCE_HANDLE(SamplerHandle);
	GFX_DEFINE_RESOURCE_HANDLE(SwapChainHandle);

	void set_error_callback(std::function<void(const char* msg)> callback);

	struct AppInfo
	{
		std::string appName;
	};
	bool initialise(const AppInfo& appInfo);
	void shutdown();

	constexpr std::uint32_t DeviceFlags_PreferDiscrete = 1u << 0u;	 // The device is typically a separate processor connected to the host
	constexpr std::uint32_t DeviceFlags_PreferIntegrated = 1u << 1u; // The device is typically one embedded or tightly couple with the host

	constexpr std::uint32_t QueueFlags_Graphics = 1u << 0u;
	constexpr std::uint32_t QueueFlags_Compute = 1u << 1u;
	constexpr std::uint32_t QueueFlags_Transfer = 1u << 2u;

	struct DeviceInfo
	{
		std::uint32_t deviceFlags;			   // Properties used to help choose a device
		std::vector<std::uint32_t> queueFlags; // The wanted queue types (The indices of the queues will be used for queue-related operations)
	};

	bool create_device(DeviceHandle& outDeviceHandle, const DeviceInfo& deviceInfo);
	void destroy_device(DeviceHandle deviceHandle);

#pragma region Device Resources

	enum class Format
	{
		eUndefined,
		eRG8,
		eRG32,
		eRGB8,
		eRGB32,
		eRGBA8,
		eRGBA32,
		eDepth16,
		eDepth24Stencil8,
		eDepth32,
		eDepth32Stencil8,
	};

	void wait_on_fence(FenceHandle fenceHandle);

	constexpr std::uint32_t CommandListFlags_FireAndForget = 1u << 0u; // Once a command list been submitted, it can no longer be reused, and it will be automatically freed (once safe to do so).

	bool create_command_list(CommandListHandle& outCommandListHandle, DeviceHandle deviceHandle, std::uint32_t queueIndex);
	void destroy_command_list(DeviceHandle deviceHandle, CommandListHandle commandListHandle);

	struct SubmitInfo
	{
		CommandListHandle commandList;
		SemaphoreHandle waitSemaphoreHandle;
	};
	void submit_command_list(const SubmitInfo& submitInfo, FenceHandle* outFenceHandle, SemaphoreHandle* outSemaphoreHandle);

	enum class DescriptorType
	{
		eStorageBuffer,
		eUniformBuffer,
		eTexture,
	};
	constexpr std::uint32_t ShaderStageFlags_Compute = 1u << 0u;
	constexpr std::uint32_t ShaderStageFlags_Vertex = 1u << 1u;
	constexpr std::uint32_t ShaderStageFlags_Fragment = 1u << 2u;
	struct DescriptorBindingInfo
	{
		DescriptorType type;
		std::uint32_t count;
		std::uint32_t shaderStages;
	};
	struct DescriptorSetInfo
	{
		std::vector<DescriptorBindingInfo> bindings{};
	};
	struct PipelineConstantBlock
	{
		std::uint32_t size;
		std::uint32_t shaderStages;
	};
	struct ComputePipelineInfo
	{
		std::vector<char> shaderCode;
		std::vector<DescriptorSetInfo> descriptorSets;
		PipelineConstantBlock constantBlock;
	};
	bool create_compute_pipeline(PipelineHandle& outPipelineHandle, DeviceHandle deviceHandle, const ComputePipelineInfo& computePipelineInfo);
	struct VertexAttribute
	{
		std::string name;
		Format format;
	};
	struct GraphicsPipelineInfo
	{
		std::vector<char> vertexCode;
		std::vector<VertexAttribute> vertexAttributes;
		std::vector<char> fragmentCode;
		std::vector<DescriptorSetInfo> descriptorSets;
		PipelineConstantBlock constantBlock;

		bool depthTest;
	};
	bool create_graphics_pipeline(PipelineHandle& outPipelineHandle, DeviceHandle deviceHandle, const GraphicsPipelineInfo& graphicsPipelineInfo);
	void destroy_pipeline(PipelineHandle pipelineHandle);

	bool create_descriptor_set_from_pipeline(DescriptorSetHandle& outDescriptorSetHandle, PipelineHandle pipelineHandle, std::uint32_t set);
	void bind_buffer_to_descriptor_set(DescriptorSetHandle descriptorSetHandle, std::uint32_t binding, BufferHandle bufferHandle);

	enum class BufferType
	{
		eVertex,
		eIndex,
		eUniform,
		eStorage,
		eUpload, // Used for uploading/copying data to GPU using command lists.
	};
	struct BufferInfo
	{
		BufferType type;
		std::uint64_t size;
	};
	bool create_buffer(BufferHandle& outBufferHandle, DeviceHandle deviceHandle, const BufferInfo& bufferInfo);
	void destroy_buffer(BufferHandle bufferHandle);
	bool map_buffer(BufferHandle bufferHandle, void*& outBufferPtr);
	void unmap_buffer(BufferHandle bufferHandle);

	enum class TextureType
	{
		e1D,
		e2D,
		e3D,
	};
	enum class TextureUsage
	{
		eTexture,
		eColorAttachment,
		eDepthStencilAttachment,
	};
	struct TextureInfo
	{
		TextureUsage usage{};
		TextureType type{};
		std::uint32_t width{};
		std::uint32_t height{};
		Format format{};
		std::uint32_t mipLevels{ 1 };
	};
	bool create_texture(TextureHandle& outTextureHandle, DeviceHandle deviceHandle, const TextureInfo& textureInfo);
	void destroy_texture(TextureHandle textureHandle);

	enum class SamplerAddressMode
	{
		eRepeat,
		eClamp,
	};
	enum class SamplerFilterMode
	{
		eLinear,
		eNearest,
	};
	struct SamplerInfo
	{
		SamplerAddressMode addressMode;
		SamplerFilterMode filterMode;
	};
	bool create_sampler(SamplerHandle& outSamplerHandle, DeviceHandle deviceHandle, const SamplerInfo& samplerInfo);
	void destroy_sampler(SamplerHandle samplerHandle);

	struct SwapChainInfo
	{
		void* platformWindowHandle; // Windows=HWND
		std::int32_t initialWidth;
		std::int32_t initialHeight;
	};
	bool create_swap_chain(SwapChainHandle& outSwapChainHandle, DeviceHandle deviceHandle, const SwapChainInfo& swapChainInfo);
	void destroy_swap_chain(SwapChainHandle swapChainHandle);
	void present_swap_chain(SwapChainHandle swapChainHandle, std::uint32_t queueIndex, SemaphoreHandle* waitSemaphore);
	bool get_swap_chain_image(TextureHandle& outTextureHandle, SwapChainHandle swapChainHandle);

#pragma endregion

#pragma region Command List Recording

	void reset(CommandListHandle commandListHandle);

	bool begin(CommandListHandle commandListHandle);
	void end(CommandListHandle commandListHandle);

	struct RenderPassInfo
	{
		std::vector<TextureHandle> colorAttachments;
		TextureHandle depthAttachment;
		std::array<float, 4> clearColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	};
	void begin_render_pass(CommandListHandle commandListHandle, const RenderPassInfo& renderPassInfo);
	void end_render_pass(CommandListHandle commandListHandle);

	void set_viewport(CommandListHandle commandListHandle, float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
	void set_scissor(CommandListHandle commandListHandle, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height);

	void bind_pipeline(CommandListHandle commandListHandle, PipelineHandle pipelineHandle);
	void bind_descriptor_set(CommandListHandle commandListHandle, DescriptorSetHandle descriptorSetHandle);
	void set_constants(CommandListHandle commandListHandle, std::uint32_t shaderStages, std::uint32_t offset, std::uint32_t size, const void* data);

	void dispatch(CommandListHandle commandListHandle, std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ);

	enum class IndexType
	{
		eUInt16,
		eUInt32,
	};
	void bind_index_buffer(CommandListHandle commandListHandle, BufferHandle bufferHandle, IndexType indexType);
	void bind_vertex_buffer(CommandListHandle commandListHandle, BufferHandle bufferHandle);

	void draw(CommandListHandle commandListHandle, std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex, std::uint32_t first_instance);
	void draw_indexed(CommandListHandle commandListHandle, std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance);

	enum class TextureState : std::uint32_t
	{
		eUndefined,
		eUploadDst, // Textures should be in this state when they are being uploaded to.
		eShaderRead,
		eRenderTarget,
		ePresent,
	};
	void transition_texture(CommandListHandle commandListHandle, TextureHandle textureHandle, TextureState oldState, TextureState newState);

	void copy_buffer_to_texture(CommandListHandle commandListHandle, BufferHandle bufferHandle, TextureHandle textureHandle);

#pragma endregion

} // namespace sm::gfx

namespace std
{
	template <>
	struct hash<sm::gfx::DescriptorSetInfo>
	{
		std::size_t operator()(const sm::gfx::DescriptorSetInfo& descriptorSetInfo)
		{
			using std::hash;
			using std::size_t;

			size_t seed{};
			sm::hash_combine(seed, descriptorSetInfo.bindings.size());
			for (const auto& binding : descriptorSetInfo.bindings)
			{
				sm::hash_combine(seed, binding.type);
				sm::hash_combine(seed, binding.count);
			}

			return seed;
		}
	};

} // namespace std

#endif // GFX_GFX_HPP
