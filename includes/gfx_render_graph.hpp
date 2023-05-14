/*
 * Copyright (c) Stuart Millman 2023.
 */

#ifndef GFX_GFX_RENDER_GRAPH_HPP
#define GFX_GFX_RENDER_GRAPH_HPP

#include <gfx.hpp>

#include <string>
#include <functional>

/*
 * Reference: https://logins.github.io/graphics/2021/05/31/RenderGraphs.html
 */
namespace sm::gfx
{
	class RenderGraphPass
	{
	public:
		void read(TextureHandle imageHandle);
		void write(TextureHandle imageHandle);

		/***
		 * @brief Define the function that gets called when the SwapChain is rebuilt (eg. resized).
		 * @param buildFunc
		 */
		void on_build(std::function<void(std::uint32_t width, std::uint32_t height)>&& buildFunc);

		/**
		 * @brief Define the function that gets called when the pass is actually executed (eg. Draw commands).
		 * @param executeFunc
		 */
		void on_execute(std::function<void(CommandListHandle commandListHandle)>&& executeFunc);
	};

	class RenderGraph
	{
	public:
		auto add_graphics_pass(const std::string& passName) -> RenderGraphPass&;

		/***
		 * @brief Compile the render graph.
		 * @return True if the render graph was successfully created.
		 */
		bool compile();

		/**
		 * @brief Execute the render graph.
		 */
		void execute(DeviceHandle deviceHandle);
	};

} // namespace sm::gfx

#endif // GFX_GFX_RENDER_GRAPH_HPP
