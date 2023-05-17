/*
 * Copyright (c) Stuart Millman 2023.
 */

#include "gfx_render_graph.hpp"

namespace sm::gfx
{
#pragma region RenderGraphPass

	void gfx::RenderGraphPass::read(TextureHandle textureHandle)
	{
		m_reads.push_back(textureHandle);
		m_readStates.push_back(TextureState::eUndefined);
	}

	void gfx::RenderGraphPass::write(TextureHandle textureHandle)
	{
		m_writes.push_back(textureHandle);
		m_writeStates.push_back(TextureState::eUndefined);
	}

	void gfx::RenderGraphPass::on_build(std::function<void(std::uint32_t, std::uint32_t)>&& buildFunc)
	{
		m_buildFunc = buildFunc;
	}

	void gfx::RenderGraphPass::on_execute(std::function<void(CommandListHandle)>&& executeFunc)
	{
		m_executeFunc = executeFunc;
	}

	void RenderGraphPass::execute(CommandListHandle commandListHandle)
	{
		m_executeFunc(commandListHandle);
	}

	void RenderGraphPass::build(std::uint32_t width, std::uint32_t height)
	{
		m_buildFunc(width, height);
	}

#pragma endregion

#pragma region RenderGraph

	auto RenderGraph::add_graphics_pass(const std::string& passName) -> RenderGraphPass&
	{
		m_passMap[passName] = std::make_unique<RenderGraphPass>();
		return *m_passMap.at(passName);
	}

	bool RenderGraph::compile()
	{
		m_executionOrder.clear();
		for (const auto& [name, pass] : m_passMap)
		{
			pass->build();
			m_executionOrder.push_back(pass.get());
		}

		return true;
	}

	void RenderGraph::execute(CommandListHandle commandListHandle)
	{
		for (auto& pass : m_executionOrder)
		{
			pass->execute(commandListHandle);
		}
	}

#pragma endregion

} // namespace sm::gfx