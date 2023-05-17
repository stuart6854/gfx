/*
 * Copyright (c) Stuart Millman 2023.
 */

#include <gfx.hpp>
#include <gfx_render_graph.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if _WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#else
	#error "Unsupported platform!"
#endif
#include <GLFW/glfw3native.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <fstream>

using namespace sm;

auto read_shader_file(const char* filename) -> std::vector<char>
{
	if (std::ifstream file{ filename, std::ios::binary | std::ios::ate })
	{
		const std::streamsize fileSize = file.tellg();
		file.seekg(0);
		std::vector<char> shaderBinary(fileSize);
		file.read(shaderBinary.data(), fileSize);
		return shaderBinary;
	}

	GFX_LOG_ERR_FMT("Example - compute - Failed to read shader file: {}", filename);
	return {};
}

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
};

bool read_obj_model(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<std::uint32_t>& outTriangles)
{
	tinyobj::ObjReaderConfig readerConfig{};
	readerConfig.mtl_search_path = "./"; // Path to material files
	readerConfig.triangulate = true;
	tinyobj::ObjReader reader{};
	if (!reader.ParseFromFile(filename, readerConfig))
	{
		if (!reader.Error().empty())
		{
			std::cerr << reader.Error() << std::endl;
		}
		return false;
	}
	if (!reader.Warning().empty())
	{
		std::cerr << reader.Warning() << std::endl;
	}

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();
	//	const auto& materials = reader.GetMaterials();

	for (const auto& shape : shapes)
	{
		std::uint64_t indexOffset{};
		for (auto faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size(); ++faceIndex)
		{
			auto faceVertexCount = shape.mesh.num_face_vertices[faceIndex];
			GFX_ASSERT(faceVertexCount == 3, "Model faces are not triangles!");

			outTriangles.push_back(outVertices.size());
			outTriangles.push_back(outVertices.size() + 1);
			outTriangles.push_back(outVertices.size() + 2);
			for (auto vertexIndex = 0; vertexIndex < faceVertexCount; ++vertexIndex)
			{
				auto& vertex = outVertices.emplace_back();

				const auto idx = shape.mesh.indices[indexOffset + vertexIndex];
				vertex.pos.x = attrib.vertices[3 * idx.vertex_index + 0];
				vertex.pos.y = attrib.vertices[3 * idx.vertex_index + 1];
				vertex.pos.z = attrib.vertices[3 * idx.vertex_index + 2];

				const bool hasNormals = idx.normal_index >= 0;
				if (hasNormals)
				{
					vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
					vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
					vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
				}

#if false
				const bool hasTexCoords = idx.texcoord_index >= 0;
				if (hasTexCoords)
				{
					const auto tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					const auto ty = attrib.texcoords[2 * idx.texcoord_index + 1];
				}
#endif
			}
			indexOffset += faceVertexCount;
		}
	}

	return true;
}

struct UniformData
{
	glm::mat4 projMat;
	glm::mat4 viewMat;
};

int main()
{
	constexpr std::uint32_t WINDOW_WIDTH = 640;
	constexpr std::uint32_t WINDOW_HEIGHT = 480;
	constexpr float WINDOW_ASPECT_RATIO = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	auto* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Render Graph", nullptr, nullptr);

	gfx::set_error_callback([](const char* msg) {
		GFX_LOG_ERR(msg);
		GFX_ASSERT(false, "");
	});

	gfx::AppInfo appInfo{ "Render Graph App" };
	if (!gfx::initialise(appInfo))
	{
		throw std::runtime_error("Failed to initialise GFX!");
	}

	gfx::DeviceInfo device_info{
		.deviceFlags = gfx::DeviceFlags_PreferDiscrete,
		.queueFlags = { gfx::QueueFlags_Graphics },
	};
	gfx::DeviceHandle deviceHandle{};
	if (!gfx::create_device(deviceHandle, device_info))
	{
		throw std::runtime_error("Failed to create GFX device!");
	}

	void* platformWindowHandle{ nullptr };
#if _WIN32
	platformWindowHandle = glfwGetWin32Window(window);
#endif

	gfx::SwapChainInfo swapChainInfo{
		.platformWindowHandle = platformWindowHandle,
		.initialWidth = WINDOW_WIDTH,
		.initialHeight = WINDOW_HEIGHT,
	};
	gfx::SwapChainHandle swapChainHandle{};
	if (!gfx::create_swap_chain(swapChainHandle, deviceHandle, swapChainInfo))
	{
		throw std::runtime_error("Failed to create GFX swap chain!");
	}

	const auto vertShaderBinary = read_shader_file("model.vert.spv");
	const auto fragShaderBinary = read_shader_file("model.frag.spv");
	gfx::GraphicsPipelineInfo pipelineInfo{
		.vertexCode = vertShaderBinary,
		.vertexAttributes = {
			{ "Position", gfx::Format::eRGB32 },
			{ "Normal", gfx::Format::eRGB32 },
		},
		.fragmentCode = fragShaderBinary,
		.descriptorSets = {
			gfx::DescriptorSetInfo{ .bindings = {
										{ gfx::DescriptorType::eUniformBuffer, 1, gfx::ShaderStageFlags_Vertex },
									} },
		},
		.constantBlock = { sizeof(glm::mat4), gfx::ShaderStageFlags_Vertex },
		.depthTest = true,
	};
	gfx::PipelineHandle pipelineHandle{};
	if (!gfx::create_graphics_pipeline(pipelineHandle, deviceHandle, pipelineInfo))
	{
		throw std::runtime_error("Failed to create GFX graphics pipeline!");
	}

	gfx::BufferInfo uniformBufferInfo{
		.type = gfx::BufferType::eUniform,
		.size = sizeof(UniformData),
	};
	gfx::BufferHandle uniformBufferHandle{};
	if (!gfx::create_buffer(uniformBufferHandle, deviceHandle, uniformBufferInfo))
	{
		throw std::runtime_error("Failed to create GFX uniform buffer!");
	}

	UniformData uniformData{
		.projMat = glm::perspectiveLH(glm::radians(60.0f), WINDOW_ASPECT_RATIO, 0.1f, 100.0f),
		.viewMat = glm::lookAtLH(glm::vec3(-1, 2, -2), glm::vec3(0, 0.5f, 0), glm::vec3(0, 1, 0)),
	};
	void* bufferPtr{ nullptr };
	if (gfx::map_buffer(uniformBufferHandle, bufferPtr))
	{
		std::memcpy(bufferPtr, &uniformData, sizeof(UniformData));
		gfx::unmap_buffer(uniformBufferHandle);
	}

	gfx::DescriptorSetHandle descriptorSetHandle{};
	if (!gfx::create_descriptor_set_from_pipeline(descriptorSetHandle, pipelineHandle, 0))
	{
		throw std::runtime_error("Failed to create GFX descriptor set!");
	}
	gfx::bind_buffer_to_descriptor_set(descriptorSetHandle, 0, uniformBufferHandle);

#pragma region Vertex/Index Buffers
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> triangles{};
	if (!read_obj_model("./stanford-bunny.obj", vertices, triangles))
	{
		throw std::runtime_error("Failed to load OBJ model!");
	}

	gfx::BufferInfo vertexBufferInfo{
		.type = gfx::BufferType::eVertex,
		.size = sizeof(Vertex) * vertices.size(),
	};
	gfx::BufferHandle vertexBufferHandle{};
	if (!gfx::create_buffer(vertexBufferHandle, deviceHandle, vertexBufferInfo))
	{
		throw std::runtime_error("Failed to create GFX vertex buffer!");
	}

	void* vertexBufferPtr{ nullptr };
	if (gfx::map_buffer(vertexBufferHandle, vertexBufferPtr))
	{
		std::memcpy(vertexBufferPtr, vertices.data(), vertexBufferInfo.size);
		gfx::unmap_buffer(vertexBufferHandle);
	}

	gfx::BufferInfo indexBufferInfo{
		.type = gfx::BufferType::eIndex,
		.size = sizeof(std::uint32_t) * triangles.size(),
	};
	gfx::BufferHandle indexBufferHandle{};
	if (!gfx::create_buffer(indexBufferHandle, deviceHandle, indexBufferInfo))
	{
		throw std::runtime_error("Failed to create GFX index buffer!");
	}

	void* indexBufferPtr{ nullptr };
	if (gfx::map_buffer(indexBufferHandle, indexBufferPtr))
	{
		std::memcpy(indexBufferPtr, triangles.data(), indexBufferInfo.size);
		gfx::unmap_buffer(indexBufferHandle);
	}
#pragma endregion

	gfx::TextureInfo shadowAttachmentInfo{
		.usage = gfx::TextureUsage::eColorAttachment,
		.type = gfx::TextureType::e2D,
		.width = WINDOW_WIDTH,
		.height = WINDOW_HEIGHT,
		.format = gfx::Format::eRGBA8, // #TODO: Make eR32.
	};
	gfx::TextureHandle shadowAttachmentHandle{};
	if (!gfx::create_texture(shadowAttachmentHandle, deviceHandle, shadowAttachmentInfo))
	{
		throw std::runtime_error("Failed to create GFX texture for shadow attachment!");
	}

	gfx::TextureInfo mainAttachmentInfo{
		.usage = gfx::TextureUsage::eColorAttachment,
		.type = gfx::TextureType::e2D,
		.width = WINDOW_WIDTH,
		.height = WINDOW_HEIGHT,
		.format = gfx::Format::eRGBA8,
	};
	gfx::TextureHandle mainAttachmentHandle{};
	if (!gfx::create_texture(mainAttachmentHandle, deviceHandle, mainAttachmentInfo))
	{
		throw std::runtime_error("Failed to create GFX texture for main attachment!");
	}

	gfx::TextureInfo depthAttachmentInfo{
		.usage = gfx::TextureUsage::eDepthStencilAttachment,
		.type = gfx::TextureType::e2D,
		.width = WINDOW_WIDTH,
		.height = WINDOW_HEIGHT,
		.format = gfx::Format::eDepth16,
	};
	gfx::TextureHandle depthAttachmentHandle{};
	if (!gfx::create_texture(depthAttachmentHandle, deviceHandle, depthAttachmentInfo))
	{
		throw std::runtime_error("Failed to create GFX texture for depth attachment!");
	}

#pragma region RenderGraph

	gfx::RenderGraph renderGraph{};
	auto& shadowPass = renderGraph.add_graphics_pass("shadowPass");
	shadowPass.write(shadowAttachmentHandle);
	shadowPass.on_build([&](std::uint32_t width, std::uint32_t height) {});
	shadowPass.on_execute([&](gfx::CommandListHandle commandListHandle) {
		gfx::bind_pipeline(commandListHandle, pipelineHandle);
		gfx::bind_descriptor_set(commandListHandle, descriptorSetHandle);

		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::scale(modelMat, glm::vec3(8, 8, 8));
		gfx::set_constants(commandListHandle, gfx::ShaderStageFlags_Vertex, 0, sizeof(glm::mat4), glm::value_ptr(modelMat));

		gfx::bind_index_buffer(commandListHandle, indexBufferHandle, gfx::IndexType::eUInt32);
		gfx::bind_vertex_buffer(commandListHandle, vertexBufferHandle);

		gfx::draw_indexed(commandListHandle, triangles.size(), 1, 0, 0, 0);
	});

	auto& mainPass = renderGraph.add_graphics_pass("mainPass");
	mainPass.write(mainAttachmentHandle);
	mainPass.write(depthAttachmentHandle);
	mainPass.read(shadowAttachmentHandle);
	mainPass.on_build([&](std::uint32_t width, std::uint32_t height) {
		//		gfx::resize_texture(mainAttachmentHandle, width, height);

		// pass.add_pipeline(); ??
	});
	mainPass.on_execute([&](gfx::CommandListHandle commandListHandle) {
		gfx::bind_pipeline(commandListHandle, pipelineHandle);
		gfx::bind_descriptor_set(commandListHandle, descriptorSetHandle);

		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::scale(modelMat, glm::vec3(8, 8, 8));
		gfx::set_constants(commandListHandle, gfx::ShaderStageFlags_Vertex, 0, sizeof(glm::mat4), glm::value_ptr(modelMat));

		gfx::bind_index_buffer(commandListHandle, indexBufferHandle, gfx::IndexType::eUInt32);
		gfx::bind_vertex_buffer(commandListHandle, vertexBufferHandle);

		gfx::draw_indexed(commandListHandle, triangles.size(), 1, 0, 0, 0);
	});

	renderGraph.compile();

#pragma endregion

	gfx::CommandListHandle commandListHandle{};
	if (!gfx::create_command_list(commandListHandle, deviceHandle, 0))
	{
		throw std::runtime_error("Failed to create GFX command list!");
	}

	double lastFrameTime = glfwGetTime();
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::scale(modelMat, glm::vec3(8, 8, 8));
	while (glfwWindowShouldClose(window) == 0)
	{
		auto time = glfwGetTime();
		auto deltaTime = static_cast<float>(time - lastFrameTime);
		lastFrameTime = time;

		glfwPollEvents();

		modelMat = glm::rotate(modelMat, glm::radians(45.0f) * deltaTime, glm::vec3(0, 1, 0));

		gfx::reset(commandListHandle);
		gfx::begin(commandListHandle);

		renderGraph.execute(commandListHandle);

		gfx::TextureHandle swapChainImageHandle{};
		if (!gfx::get_swap_chain_image(swapChainImageHandle, swapChainHandle))
		{
			throw std::runtime_error("Failed to get SwapChain image handle!");
		}

		gfx::transition_texture(commandListHandle, swapChainImageHandle, gfx::TextureState::eUndefined, gfx::TextureState::eRenderTarget);

		gfx::RenderPassInfo renderPassInfo{
			.colorAttachments = { swapChainImageHandle },
			.depthAttachment = depthAttachmentHandle,
			.clearColor = { 0.392f, 0.584f, 0.929f, 1.0f }, // Cornflower Blue
		};
		gfx::begin_render_pass(commandListHandle, renderPassInfo);
		{
			gfx::set_viewport(commandListHandle, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
			gfx::set_scissor(commandListHandle, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

			gfx::bind_pipeline(commandListHandle, pipelineHandle);
			gfx::bind_descriptor_set(commandListHandle, descriptorSetHandle);
			gfx::set_constants(commandListHandle, gfx::ShaderStageFlags_Vertex, 0, sizeof(glm::mat4), glm::value_ptr(modelMat));

			gfx::bind_index_buffer(commandListHandle, indexBufferHandle, gfx::IndexType::eUInt32);
			gfx::bind_vertex_buffer(commandListHandle, vertexBufferHandle);

			gfx::draw_indexed(commandListHandle, triangles.size(), 1, 0, 0, 0);
		}
		gfx::end_render_pass(commandListHandle);

		gfx::transition_texture(commandListHandle, swapChainImageHandle, gfx::TextureState::eRenderTarget, gfx::TextureState::ePresent);

		gfx::end(commandListHandle);

		gfx::SubmitInfo submitInfo{
			.commandList = commandListHandle,
			.waitSemaphoreHandle = {}
		};
		gfx::FenceHandle fenceHandle;
		gfx::submit_command_list(submitInfo, &fenceHandle, nullptr);

		gfx::wait_on_fence(fenceHandle);

		gfx::present_swap_chain(swapChainHandle, 0, nullptr);
	}

	gfx::destroy_swap_chain(swapChainHandle);

	gfx::destroy_device(deviceHandle);
	gfx::shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();
}