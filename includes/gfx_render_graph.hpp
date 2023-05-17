/*
 * Copyright (c) Stuart Millman 2023.
 */

#ifndef GFX_GFX_RENDER_GRAPH_HPP
#define GFX_GFX_RENDER_GRAPH_HPP

#include <gfx.hpp>

#include <string>
#include <functional>
#include <unordered_map>

/*
 * Reference: https://logins.github.io/graphics/2021/05/31/RenderGraphs.html
 */
namespace sm::gfx
{
	class RenderGraphPass
	{
	public:
		void read(TextureHandle textureHandle);
		void write(TextureHandle textureHandle);

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

	private:
		friend class RenderGraph;

		void build(std::uint32_t width, std::uint32_t height);
		void execute(CommandListHandle commandListHandle);

	private:
		std::vector<TextureHandle> m_reads;
		std::vector<TextureState> m_readStates;

		std::vector<TextureHandle> m_writes;
		std::vector<TextureState> m_writeStates;

		std::function<void(std::uint32_t width, std::uint32_t height)> m_buildFunc;
		std::function<void(CommandListHandle commandListHandle)> m_executeFunc;
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
		void execute(CommandListHandle commandListHandle);

	private:
		std::unordered_map<std::string, std::unique_ptr<RenderGraphPass>> m_passMap;

		std::vector<RenderGraphPass*> m_executionOrder; // Should be decided by the end of compilation.
	};

} // namespace sm::gfx

#endif // GFX_GFX_RENDER_GRAPH_HPP
