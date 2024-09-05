#include  "ModelsLoader.h"


int main()
{

    std::filesystem::path cwd = std::filesystem::current_path();

	
	const std::filesystem::path sponza_path{"Sponza\\glTF\\Sponza.gltf"};
	std::unique_ptr<Anni::ModelLoader::LoadedModel> sponza = Anni::ModelLoader::LoadFromFile(sponza_path);

	return 0;
}