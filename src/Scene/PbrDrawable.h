#pragma once

#include <filesystem>
#include <vector>

#include "Util/Drawable.h"
#include "Util/Loader.h"
#include "Util/Mesh.h"

class PbrDrawable : public Drawable {
public:
	PbrDrawable(DXContext* context, RenderPipeline* pipeline);

	void draw(Camera* camera);

	size_t getSceneSize();

	void releaseResources() override;

	bool instanced{ false };

private:
	DXContext* context;
	RenderPipeline* renderPipeline;

	std::vector<Mesh> meshes;
	std::vector<XMFLOAT4X4> modelMatrices;

	size_t sceneSize{ 0 };

	GltfData gltfData;

	//Texture envMap;

	void construct();
};