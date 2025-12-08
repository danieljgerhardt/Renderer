#pragma once

#include <filesystem>
#include <vector>

#include "Drawable.h"
#include "Loader.h"
#include "Mesh.h"

class ObjectScene : public Drawable {
public:
	ObjectScene(DXContext* context, RenderPipeline* pipeline);

	void constructSceneSolid();

	void draw(Camera* camera);

	size_t getSceneSize();

	void releaseResources();

	bool instanced{ false };

private:
	std::vector<Mesh> meshes;
	std::vector<XMFLOAT4X4> modelMatrices;

	size_t sceneSize{ 0 };

	GltfData gltfData;
};