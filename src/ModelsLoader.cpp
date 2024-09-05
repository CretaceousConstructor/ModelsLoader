#define STB_IMAGE_IMPLEMENTATION
#include "ModelsLoader.h"
#include "stb_image.h"            //TODO: stb image need some marco to work

Anni::ModelLoader::LoadedModel::Factory  Anni::ModelLoader::LoadedModel::factory{};
namespace Anni::ModelLoader
{

	void LoadedModel::Factory::LoadGltf(const std::filesystem::path& file_path, fastgltf::Parser& gltf_parser, std::unique_ptr<LoadedModel>& loading_result)
	{
		fastgltf::Asset gltf_asset{};
		fastgltf::GltfDataBuffer data{};

		LoadRawGltf(data, gltf_asset, file_path, gltf_parser);
		LoadSamplers(gltf_asset, loading_result);
		LoadTextureImages(gltf_asset, file_path, loading_result);
		LoadMaterials(gltf_asset, file_path, loading_result);
		LoadMeshes(gltf_asset, loading_result);
		LoadSceneNodes(gltf_asset, loading_result);
	}


	void LoadedModel::Factory::LoadRawGltf(fastgltf::GltfDataBuffer& data, fastgltf::Asset& gltf_asset, const std::filesystem::path& file_path, fastgltf::Parser& gltf_parser)
	{


		//> LOAD_RAW GLTF RAW FILE LOADING
		constexpr auto gltf_loading_options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers;

		auto result_buffer = fastgltf::GltfDataBuffer::FromPath(file_path);

		if ( !result_buffer )
		{
			SPDLOG_ERROR("Failed to load data buffer from given file path.");
			std::exit(EXIT_FAILURE); // or std::exit(1);
		}
		data = std::move(result_buffer.get());
		const fastgltf::GltfType gltf_type = determineGltfFileType(data);

		if ( fastgltf::GltfType::glTF == gltf_type )
		{
			auto load = gltf_parser.loadGltf(data, file_path.parent_path(), gltf_loading_options);
			SPDLOG_INFO("Parsing file from gltf directory: {}", file_path.parent_path().string());
			if ( load )
			{
				gltf_asset = std::move(load.get());
			}
			else
			{
				SPDLOG_ERROR("Failed to load GLTF file.");
				std::exit(EXIT_FAILURE); // or std::exit(1);
			}
		}
		else if ( fastgltf::GltfType::GLB == gltf_type )
		{
			auto load = gltf_parser.loadGltfBinary(data, file_path.parent_path(), gltf_loading_options);

			if ( load )
			{
				gltf_asset = std::move(load.get());
			}
			else
			{
				SPDLOG_ERROR("Failed to load GLB file.");
				std::exit(EXIT_FAILURE); // or std::exit(1);
			}
		}
		else
		{
			SPDLOG_ERROR("Unknown type of model file.");
			std::exit(EXIT_FAILURE); // or std::exit(1);
		}
	}

	void LoadedModel::Factory::LoadSamplers(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result)
	{
		loading_result->m_samplers.reserve(gltf_asset.samplers.size());
		for ( const fastgltf::Sampler& sampler : gltf_asset.samplers )
		{
			LoadedSampler loaded_sampler{};
			loaded_sampler.mag = ExtractMagSamplerType(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
			loaded_sampler.min = ExtractMinSamplerType(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
			loaded_sampler.mipmap = ExtractMipMapSamplerType(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

			loaded_sampler.u_mode = ExtractAddressMode(sampler.wrapS);
			loaded_sampler.v_mode = ExtractAddressMode(sampler.wrapT);

			loading_result->m_samplers.push_back(loaded_sampler);
		}
	}

	LoadedSampler::AddressMode LoadedModel::Factory::ExtractAddressMode(const fastgltf::Wrap warp)
	{
		//read the spec, found out this is the way they define this
		switch ( warp )
		{
			case fastgltf::Wrap::ClampToEdge:
				return LoadedSampler::AddressMode::ClampToEdge;
			case fastgltf::Wrap::MirroredRepeat:
				return LoadedSampler::AddressMode::MirroredRepeat;
			case fastgltf::Wrap::Repeat:
				return LoadedSampler::AddressMode::Repeat;
		}
	}

	LoadedSampler::SamplerType LoadedModel::Factory::ExtractMagSamplerType(const fastgltf::Filter filter)
	{
		//read the spec, found out this is the way they define this
		switch ( filter )
		{
			case fastgltf::Filter::Nearest:
				return LoadedSampler::SamplerType::Nearest;
			case fastgltf::Filter::Linear:
				return LoadedSampler::SamplerType::Linear;
			default:
				return LoadedSampler::SamplerType::Nearest;
		}
	}

	LoadedSampler::SamplerType LoadedModel::Factory::ExtractMinSamplerType(const fastgltf::Filter filter)
	{
		switch ( filter )
		{
			case fastgltf::Filter::Nearest:
			case fastgltf::Filter::NearestMipMapNearest:
			case fastgltf::Filter::NearestMipMapLinear:
				return LoadedSampler::SamplerType::Nearest;

			case fastgltf::Filter::Linear:
			case fastgltf::Filter::LinearMipMapNearest:
			case fastgltf::Filter::LinearMipMapLinear:
				return LoadedSampler::SamplerType::Linear;
		}
	}

	LoadedSampler::SamplerType LoadedModel::Factory::ExtractMipMapSamplerType(const fastgltf::Filter filter)
	{
		switch ( filter )
		{
			case fastgltf::Filter::Nearest:
			case fastgltf::Filter::NearestMipMapNearest:
			case fastgltf::Filter::LinearMipMapNearest:
				return LoadedSampler::SamplerType::Nearest;

			case fastgltf::Filter::Linear:
			case fastgltf::Filter::NearestMipMapLinear:
			case fastgltf::Filter::LinearMipMapLinear:
				return LoadedSampler::SamplerType::Linear;
		}
	}

	void LoadedModel::Factory::LoadTextureImages(const fastgltf::Asset& gltf_asset, const std::filesystem::path& file_path, std::unique_ptr<LoadedModel>& loading_result)
	{
		loading_result->m_textures.reserve(gltf_asset.images.size());
		//> LOAD ALL TEXTURES
		for ( const auto& image : gltf_asset.images )
		{
			const std::string img_name = image.name.c_str();
			loading_result->m_textures.emplace_back(img_name);
			LoadedImage& loaded_image = loading_result->m_textures.back();

			int width = 0, height = 0, num_channels = 0;
			{
				if ( std::get_if<fastgltf::sources::URI>(&image.data) )
				{
					const fastgltf::sources::URI* p_img_loca_Path_URI = std::get_if<fastgltf::sources::URI>(&image.data);
					const auto& img_loca_path_URI = *p_img_loca_Path_URI;

					const std::string img_local_uri = img_loca_path_URI.uri.string().data();
					loading_result->m_textures.back().file_name.value().append(img_local_uri);


					if ( img_loca_path_URI.fileByteOffset != 0 ) // We don't support offsets with stbi.
					{
						SPDLOG_ERROR("Don't support offsets with stbi.");
						std::exit(EXIT_FAILURE); // or std::exit(1);
					}

					if ( !img_loca_path_URI.uri.isLocalPath() ) // We're only capable of loading local files.
					{
						SPDLOG_ERROR("Only capable of loading local files.");
						std::exit(EXIT_FAILURE); // or std::exit(1);
					}

					const std::string img_local_path(
						img_loca_path_URI.uri.path().begin(),
						img_loca_path_URI.uri.path().end()); // Thanks C++.

					const std::filesystem::path absolute_path = file_path.parent_path().append(img_local_path);

					constexpr int desired_components = 4;
					unsigned char* const temp_tex_data = stbi_load(absolute_path.generic_string().c_str(), &width,
																   &height, &num_channels, desired_components);

					if ( !(num_channels == 4 || num_channels == 3) )
					{
						SPDLOG_ERROR("Unsupported number of channels.");
						std::exit(EXIT_FAILURE); // or std::exit(1);
					}
					loaded_image.num_channels = num_channels;
					loaded_image.mipmap_size = 1;
					loaded_image.array_size = 1;
					if ( temp_tex_data )
					{
						const uint64_t tex_width = width;
						const uint64_t tex_height = height;
						constexpr uint16_t depth_or_array_size = 1;

						loaded_image.raw_data.resize(tex_height * tex_height * desired_components);
						// Copy data from temp_tex_data to raw_data
						memcpy(loaded_image.raw_data.data(), temp_tex_data, width * height * desired_components);
					}
					//free the image
					stbi_image_free(temp_tex_data);
				}
				else
				{
					SPDLOG_ERROR("Haven't implemented.");
					std::exit(EXIT_FAILURE); // or std::exit(1);
				}
			}
		}
	}

	void LoadedModel::Factory::LoadMaterials(const fastgltf::Asset& gltf_asset, const std::filesystem::path& file_path, std::unique_ptr<LoadedModel>& loading_result)
	{
		loading_result->m_materials.reserve(gltf_asset.materials.size());
		for ( const auto& mat : gltf_asset.materials )
		{
			LoadedMaterialConstant constants;
			constants.base_color_factors[0] = mat.pbrData.baseColorFactor[0];
			constants.base_color_factors[1] = mat.pbrData.baseColorFactor[1];
			constants.base_color_factors[2] = mat.pbrData.baseColorFactor[2];
			constants.base_color_factors[3] = mat.pbrData.baseColorFactor[3];

			constants.metallic_factor = mat.pbrData.metallicFactor;
			constants.roughness_factor = mat.pbrData.roughnessFactor;

			switch ( mat.alphaMode )
			{
				case fastgltf::AlphaMode::Opaque:
					constants.alpha_mode = LoadedMaterialConstant::AlphaMode::Opaque;
				case fastgltf::AlphaMode::Blend:
					constants.alpha_mode = LoadedMaterialConstant::AlphaMode::Blend;
				case fastgltf::AlphaMode::Mask:
					constants.alpha_mode = LoadedMaterialConstant::AlphaMode::Mask;
					constants.alpha_cutoff = mat.alphaCutoff;
			}

			// install m_textures index
			if ( mat.pbrData.baseColorTexture.has_value() )
			{
				constants.albedo_index = gltf_asset.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
				constants.albedo_sampler_index = gltf_asset.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();
			}

			if ( mat.pbrData.metallicRoughnessTexture.has_value() )
			{
				constants.metal_roughness_index = gltf_asset.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
				constants.metal_roughness_sampler_index = gltf_asset.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();
			}

			if ( mat.normalTexture.has_value() )
			{
				constants.normal_index = gltf_asset.textures[mat.normalTexture.value().textureIndex].imageIndex.value();
				constants.normal_sampler_index = gltf_asset.textures[mat.normalTexture.value().textureIndex].samplerIndex.value();
			}

			if ( mat.emissiveTexture.has_value() )
			{
				constants.emissive_index = gltf_asset.textures[mat.emissiveTexture.value().textureIndex]
					.imageIndex.value();
				constants.emissive_sampler_index = gltf_asset.textures[mat.emissiveTexture.value().textureIndex]
					.samplerIndex.value();
			}

			if ( mat.occlusionTexture.has_value() )
			{
				constants.occlusion_index = gltf_asset.textures[mat.occlusionTexture.value().textureIndex]
					.imageIndex.value();
				constants.occlusion_sampler_index = gltf_asset.textures[mat.occlusionTexture.value().textureIndex]
					.samplerIndex.value();
			}
			loading_result->m_materials.push_back(constants);
		}
	}

	void LoadedModel::Factory::LoadMeshes(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result)
	{
		// use the same vectors for all meshes so that the memory doesn't reallocate  as often
		std::vector<uint32_t> indices;
		std::vector<LoadedVertex> vertices;

		loading_result->m_mesh_assets.resize(gltf_asset.meshes.size());

		for ( auto [mesh_index, mesh] : std::ranges::views::enumerate(gltf_asset.meshes) )
		{
			std::string mesh_name{ mesh.name };
			loading_result->m_mesh_assets[mesh_index].name = mesh_name.append(std::to_string(mesh_index));

			// clear the mesh_asset arrays each mesh_asset, we don't want to merge them by error
			indices.clear();
			vertices.clear();

			// process homo mat tris
			for ( auto&& primitive : mesh.primitives )
			{
				LoadedMeshAsset::HomoMatTris homo_mat_tris;
				homo_mat_tris.start_index = static_cast< uint32_t >(indices.size());
				homo_mat_tris.count = static_cast< uint32_t >(gltf_asset.accessors[primitive.indicesAccessor.value()].count);

				size_t initial_vtx = vertices.size();

				// load indexes
				{
					const fastgltf::Accessor& indexaccessor = gltf_asset.accessors[primitive.indicesAccessor.value()];
					indices.reserve(indices.size() + indexaccessor.count);

					fastgltf::iterateAccessor<std::uint32_t>(
						gltf_asset, indexaccessor,
						[&](std::uint32_t idx)
						{
							indices.push_back(idx + initial_vtx);
						});
				}

				// load vertex positions
				{
					const fastgltf::Accessor& posAccessor = gltf_asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
					vertices.resize(vertices.size() + posAccessor.count);

					fastgltf::iterateAccessorWithIndex<fastgltf::math::vec<float, 3>>(
						gltf_asset, posAccessor,
						[&](fastgltf::math::vec<float, 3> v, size_t index) -> void
						{
							LoadedVertex newvtx;
							newvtx.position = glm::vec3(v[0], v[1], v[2]);
							newvtx.normal = { 0, 1, 0 };
							newvtx.color = glm::vec4{ 1.f };
							newvtx.uv = glm::vec2{ 0.f, 0.f };
							newvtx.tangent = { 1, 0, 0, 0 };
							vertices[initial_vtx + index] = newvtx;
						});
				}

				// load vertex normals
				const auto normals = primitive.findAttribute("NORMAL");
				if ( normals != primitive.attributes.end() )
				{
					fastgltf::iterateAccessorWithIndex<fastgltf::math::vec<float, 3>>
						(
							gltf_asset, gltf_asset.accessors[normals->accessorIndex],
							[&](fastgltf::math::vec<float, 3> v, size_t index)
							{
								vertices[initial_vtx + index].normal = glm::vec3(v[0], v[1], v[2]);
							}
						);
				}

				// load UVs
				const auto uv = primitive.findAttribute("TEXCOORD_0");
				if ( uv != primitive.attributes.end() )
				{
					fastgltf::iterateAccessorWithIndex<fastgltf::math::vec<float, 2>>(
						gltf_asset, gltf_asset.accessors[uv->accessorIndex],
						[&](fastgltf::math::vec<float, 2> v, size_t index)
						{
							vertices[initial_vtx + index].uv.x = v[0];
							vertices[initial_vtx + index].uv.y = v[1];
						});
				}

				// load vertex colors
				const auto colors = primitive.findAttribute("COLOR_0");
				if ( colors != primitive.attributes.end() )
				{
					fastgltf::iterateAccessorWithIndex<fastgltf::math::vec<float, 4>>(
						gltf_asset, gltf_asset.accessors[colors->accessorIndex],
						[&](fastgltf::math::vec<float, 4> v, size_t index)
						{
							vertices[initial_vtx + index].color = glm::vec4(v[0], v[1], v[2], v[3]);
						});
				}

				// load tangent
				const auto tangent = primitive.findAttribute("TANGENT");
				if ( tangent != primitive.attributes.end() )
				{
					fastgltf::iterateAccessorWithIndex<fastgltf::math::vec<float, 4>>(
						gltf_asset, gltf_asset.accessors[tangent->accessorIndex],
						[&](fastgltf::math::vec<float, 4> v, size_t index)
						{
							vertices[initial_vtx + index].tangent = glm::vec4(v[0], v[1], v[2], v[3]);
						});
				}

				// load material index
				if ( primitive.materialIndex.has_value() )
				{
					homo_mat_tris.material_index = primitive.materialIndex.value();
				}
				else
				{
					//SPD log here
					SPDLOG_ERROR("No material index is specified for the current homo-material triangles.");
					std::exit(EXIT_FAILURE); // or std::exit(1);
				}

				loading_result->m_mesh_assets[mesh_index].homo_mat_tris_array.push_back(homo_mat_tris);
			}
			loading_result->m_mesh_assets[mesh_index].buffer_in_one.indices = std::move(indices);
			loading_result->m_mesh_assets[mesh_index].buffer_in_one.vertices = std::move(vertices);
		}
	}

	void LoadedModel::Factory::LoadSceneNodes(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result)
	{
		// LOAD ALL NODES AND THEIR MESHES
		for ( auto [node_index, node] : std::ranges::views::enumerate(gltf_asset.nodes) )
		{
			std::shared_ptr<Node> new_node;

			// find if the node has a mesh_asset, and if it does then hook it to the mesh_asset
			// pointer and allocate it with the meshnode class
			if ( node.meshIndex.has_value() )
			{
				new_node = std::make_shared<MeshNode>(&loading_result->m_mesh_assets[node.meshIndex.value()]);
			}
			else
			{
				//SPD log here
				SPDLOG_ERROR("The given mesh node doesn't have a mesh asset associated with it.");
				std::exit(EXIT_FAILURE); // or std::exit(1);
			}

			loading_result->m_scene_nodes.push_back(new_node);

			std::visit
			(
				fastgltf::visitor
				{
					[&](const fastgltf::math::fmat4x4& matrix)
					{
						memcpy(&new_node->local_transform, matrix.data(), sizeof(matrix));
					},

					[&](const fastgltf::TRS& transform)
					{
						const glm::vec3 tl(
							transform.translation[0],
							transform.translation[1],
							transform.translation[2]);
						const glm::quat rot(
							transform.rotation[3],
							transform.rotation[0],
							transform.rotation[1],
							transform.rotation[2]);
						const glm::vec3 sc(
							transform.scale[0],
							transform.scale[1],
							transform.scale[2]);

						const glm::mat4 tm = translate(glm::mat4(1.f), tl);
						const glm::mat4 rm = mat4_cast(rot);
						const glm::mat4 sm = scale(glm::mat4(1.f), sc);
						new_node->local_transform = tm * rm * sm;
					}
				},
				node.transform
			);
		}
	}


	void LoadedModel::Factory::LoadSceneGraph(const fastgltf::Asset& gltf_asset, std::unique_ptr<LoadedModel>& loading_result)
	{
		//> LOAD_SCENE_GRAPH
		// run loop again to setup scene graph hierarchy and refresh transform
		for ( auto [node_index, node_from_gltf] : std::ranges::views::enumerate(gltf_asset.nodes) )
		{
			const std::shared_ptr<Node>& already_loaded_node = loading_result->m_scene_nodes[node_index];

			for ( auto& c : node_from_gltf.children )
			{
				already_loaded_node->children.push_back(loading_result->m_scene_nodes[c]);
				loading_result->m_scene_nodes[c]->parent = already_loaded_node;
			}
		}

		// find the top nodes, with no parents
		for ( auto& node : loading_result->m_scene_nodes )
		{
			if ( !node->parent.lock() )
			{
				loading_result->m_top_nodes.push_back(node);
				node->RefreshTransform(glm::mat4{ 1.f });
			}
		}
		//< load_scene_graph
	}
}


Anni::ModelLoader::LoadedImage::LoadedImage(std::string file_name_) :
	file_name(std::move(file_name_))
{
}

Anni::ModelLoader::Node::Node() : IRenderable(), local_transform(), world_transform()
{
}

void Anni::ModelLoader::Node::RefreshTransform(const glm::mat4& parent_matrix)
{
	world_transform = parent_matrix * local_transform;
	for ( const auto& c : children )
	{
		c->RefreshTransform(world_transform);
	}
}

void Anni::ModelLoader::Node::PreDrawToContext(const glm::mat4& top_matrix, DrawContext& ctx)
{
	// draw children
	for ( const auto& c : children )
	{
		c->PreDrawToContext(top_matrix, ctx);
	}
}

Anni::ModelLoader::MeshNode::MeshNode(const LoadedMeshAsset* const mesh_asset_) :
	Node(),
	mesh_asset(mesh_asset_)
{
}

void Anni::ModelLoader::MeshNode::PreDrawToContext(const glm::mat4& top_matrix, DrawContext& ctx)
{
	const glm::mat4 node_matrix = top_matrix * world_transform;
	for ( auto& homo_mat_tris : mesh_asset->homo_mat_tris_array )
	{
		RenderRecord def;
		def.index_count = homo_mat_tris.count;
		def.first_index = homo_mat_tris.start_index;
		def.material_index = homo_mat_tris.material_index;
		def.final_transform = node_matrix;

		ctx.homo_mat_tris_record.push_back(def);
	}

	// recurse down(call PreDrawToContext() as Node(i.e. as base class's virtual function), which iterate the Node's all children Nodes.)
	Node::PreDrawToContext(top_matrix, ctx);
}

Anni::ModelLoader::LoadedModel::LoadedModel(std::filesystem::path file_path) : m_file_path(std::move(file_path))
{
}


std::unique_ptr<Anni::ModelLoader::LoadedModel> Anni::ModelLoader::LoadedModel::Factory::LoadFromFile(const std::filesystem::path file_path)
{
	if ( !file_path.has_extension() )
	{
		SPDLOG_ERROR("Provided file path doesn't have a file extension!");
		std::exit(EXIT_FAILURE); // or std::exit(1);
	}

	SPDLOG_INFO("Loading file from the file path: {}", file_path.string());

	const auto raw_ptr_loading_result = new LoadedModel(file_path);
	std::unique_ptr<LoadedModel> loading_result(raw_ptr_loading_result);


	fastgltf::Parser gltf_parser{};

	const std::string extension = file_path.extension().string();
	if ( ".gltf" == extension )
	{
		LoadGltf(file_path, gltf_parser, loading_result);
	}

	return loading_result;
}
