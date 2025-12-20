#pragma once

#include <filesystem>
#include <vector>

#include "Util/Drawable.h"
#include "Util/Loader.h"
#include "Util/Mesh.h"

class ObjectScene : public Drawable {
public:
	ObjectScene(DXContext* context, RenderPipeline* pipeline);

	void draw(Camera* camera) override;

	size_t getSceneSize();

	void releaseResources() override;

	bool instanced{ false };

private:
	std::vector<Mesh*> meshes;
	std::vector<XMFLOAT4X4> modelMatrices;

	size_t sceneSize{ 0 };

	GltfData gltfData;

    void constructScene();
};