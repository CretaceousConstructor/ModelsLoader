#pragma once
#include <filesystem>
#include <ranges>

#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/tools.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace Anni::ModelLoader
{
	struct LoadedSampler
	{
		enum class SamplerType : std::uint16_t
		{
			Nearest,
			Linear,
		};

		enum class AddressMode : std::uint16_t
		{
			ClampToEdge,
			MirroredRepeat,
			Repeat,
		};

		SamplerType min{ SamplerType::Nearest };
		SamplerType mag{ SamplerType::Nearest };
		SamplerType mipmap{ SamplerType::Nearest };

		AddressMode u_mode{ AddressMode::ClampToEdge };
		AddressMode v_mode{ AddressMode::ClampToEdge };
		AddressMode w_mode{ AddressMode::ClampToEdge };
	};

	struct LoadedImage
	{
		LoadedImage(std::string file_name_);
		std::optional<std::string> file_name;
		std::optional<uint32_t> array_size;
		std::optional<uint32_t> mipmap_size;
		std::optional<uint32_t> num_channels;
		std::vector<uint8_t> raw_data;
	};

	struct LoadedMaterialConstant
	{
		enum class AlphaMode :std::uint16_t
		{
			Opaque,
			Mask,
			Blend,
		};

		AlphaMode alpha_mode;
		float metallic_factor;
		float roughness_factor;
		std::optional<float> alpha_cutoff;
		std::array<float, 4> base_color_factors;

		std::optional<uint32_t> albedo_index;
		std::optional<uint32_t> albedo_sampler_index;

		std::optional<uint32_t> metal_roughness_index;
		std::optional<uint32_t> metal_roughness_sampler_index;

		std::optional<uint32_t> normal_index;
		std::optional<uint32_t> normal_sampler_index;

		std::optional<uint32_t> emissive_index;
		std::optional<uint32_t> emissive_sampler_index;

		std::optional<uint32_t> occlusion_index;
		std::optional<uint32_t> occlusion_sampler_index;
	};

	class LoadedVertex
	{
	public:
		glm::vec4 color;
		glm::vec4 tangent;
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct LoadedMeshAsset
	{
		struct HomoMatTris //multiple triangles with the same material.
		{
			uint32_t start_index;
			uint32_t count;
			std::optional<uint32_t> material_index;
		};

		struct MeshBuffer //all buffers in one 
		{
			std::vector<uint32_t> indices;
			std::vector<LoadedVertex> vertices;
		};

		std::string name;
		std::vector<HomoMatTris> homo_mat_tris_array;
		MeshBuffer buffer_in_one;
	};


	struct RenderRecord
	{
		uint32_t index_count;
		uint32_t first_index;
		std::optional<uint32_t> material_index;
		glm::mat4 final_transform;
	};

	struct DrawContext
	{
		std::vector<RenderRecord> homo_mat_tris_record;
	};

	class IRenderable
	{
	public:
		IRenderable() = default;

		virtual void PreDrawToContext(const glm::mat4& top_matrix, DrawContext& ctx) = 0;
		virtual ~IRenderable() = default;
	};

	struct Node : public IRenderable
	{
	public:
		Node();
		~Node() override = default;

		// parent pointer must be a weak pointer to avoid circular dependencies
		std::weak_ptr<Node> parent;
		std::vector<std::shared_ptr<Node>> children;

		glm::mat4 local_transform;
		glm::mat4 world_transform;

		void RefreshTransform(const glm::mat4& parent_matrix);
		void PreDrawToContext(const glm::mat4& top_matrix, DrawContext& ctx) override;
	};

	struct MeshNode : public Node
	{
	public:
		explicit MeshNode(const LoadedMeshAsset* mesh_asset_);

		MeshNode() = delete;
		~MeshNode() override = default;
		void PreDrawToContext(const glm::mat4& top_matrix, DrawContext& ctx) override;

	private:
		// OBSERVER POINTER
		const LoadedMeshAsset* const mesh_asset;
	};


	class LoadedModel
	{
	public:
		~LoadedModel() = default;
		LoadedModel() = delete;
		LoadedModel(const LoadedModel&) = delete;
		LoadedModel(LoadedModel&&) = delete;
		LoadedModel& operator=(const LoadedModel&) = delete;
		LoadedModel& operator=(LoadedModel&&) = delete;
	private:
		LoadedModel(std::filesystem::path file_path);

	private:
		class Factory
		{
		public:
			std::unique_ptr<LoadedModel> LoadFromFile(std::filesystem::path file_path);

		private:
			static void LoadGltf(const std::filesystem::path& file_path, fastgltf::Parser& gltf_parser, std::unique_ptr<LoadedModel>& loading_result);
			static void LoadRawGltf(fastgltf::GltfDataBuffer& data, fastgltf::Asset& gltf_asset, const std::filesystem::path& file_path, fastgltf::Parser& gltf_parser);
			static void LoadSamplers(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result);
			static void LoadTextureImages(const fastgltf::Asset& gltf_asset, const std::filesystem::path& file_path, std::unique_ptr<LoadedModel>& loading_result);
			static void LoadMaterials(const fastgltf::Asset& gltf_asset, const std::filesystem::path& file_path, std::unique_ptr<LoadedModel>& loading_result);
			static void LoadMeshes(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result);
			static void LoadSceneNodes(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result);
			static void LoadSceneGraph(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result);

			static LoadedSampler::AddressMode ExtractAddressMode(fastgltf::Wrap warp);
			static LoadedSampler::SamplerType ExtractMagSamplerType(fastgltf::Filter filter);
			static LoadedSampler::SamplerType ExtractMinSamplerType(fastgltf::Filter filter);
			static LoadedSampler::SamplerType ExtractMipMapSamplerType(fastgltf::Filter filter);
		};

		std::filesystem::path m_file_path;
		std::vector<LoadedSampler> m_samplers;
		std::vector<LoadedImage> m_textures;
		std::vector<LoadedMaterialConstant> m_materials;
		std::vector<LoadedMeshAsset> m_mesh_assets;
		std::vector<std::shared_ptr<Node>> m_scene_nodes;
		std::vector<std::shared_ptr<Node>> m_top_nodes;

	public:
		static Factory factory;
	};
}

// namespace Anni
