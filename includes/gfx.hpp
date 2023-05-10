/*
 * Copyright (c) Stuart Millman 2023.
 */

#ifndef GFX_GFX_HPP
#define GFX_GFX_HPP

#include <cstdint>
#include <functional>
#include <string_view>

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

namespace sm::gfx
{
	GFX_DEFINE_HANDLE(DeviceHandle);
	GFX_DEFINE_HANDLE(ResourceHandle);
	GFX_DEFINE_RESOURCE_HANDLE(CommandListHandle);
	GFX_DEFINE_RESOURCE_HANDLE(FenceHandle);
	GFX_DEFINE_RESOURCE_HANDLE(SemaphoreHandle);

	void set_error_callback(std::function<void(const char* msg)> callback);

	struct AppInfo
	{
		std::string appName;
	};
	bool initialise(const AppInfo& appInfo);
	void shutdown();

	constexpr std::uint32_t DeviceFlags_PreferDiscrete = 1 << 0;   // The device is typically a separate processor connected to the host
	constexpr std::uint32_t DeviceFlags_PreferIntegrated = 1 << 1; // The device is typically one embedded or tightly couple with the host

	constexpr std::uint32_t QueueFlags_Graphics = 1 << 0;
	constexpr std::uint32_t QueueFlags_Compute = 1 << 1;
	constexpr std::uint32_t QueueFlags_Transfer = 1 << 2;

	struct DeviceInfo
	{
		std::uint32_t deviceFlags;			   // Properties used to help choose a device
		std::vector<std::uint32_t> queueFlags; // The queues required and their capabilities
	};

	bool create_device(DeviceHandle& outDeviceHandle, const DeviceInfo& deviceInfo);
	void destroy_device(DeviceHandle deviceHandle);

	bool create_command_list(CommandListHandle& outCommandListHandle, DeviceHandle deviceHandle, std::uint32_t queueFlags);
	void destroy_command_list(DeviceHandle deviceHandle, CommandListHandle commandListHandle);

	void submit_command_list(CommandListHandle commandListHandle, FenceHandle* OutFenceHandle, SemaphoreHandle* OutSemaphoreHandle);

#pragma region Command List Recording

	bool begin(CommandListHandle commandListHandle);
	void end(CommandListHandle commandListHandle);

	void draw(CommandListHandle commandListHandle, std::uint32_t vertex_count, std::uint32_t instance_count, std::uint32_t first_vertex, std::uint32_t first_instance);
	void draw_indexed(CommandListHandle commandListHandle, std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance);

#pragma endregion

} // namespace sm::gfx

#endif // GFX_GFX_HPP
