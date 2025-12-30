#pragma once

#include <filesystem>
#include <vector>

#include "Util/Drawable.h"
#include "Util/Loader.h"
#include "Util/Mesh.h"

class EnvironmentMapDrawable : public Drawable {
public:
	EnvironmentMapDrawable(DXContext* context, RenderPipeline* pipeline, Texture* envCubeMap);

	void draw(Camera* camera, D3D12_VIEWPORT& vp) override;

	bool drawEveryFrame() override { return true; }

	size_t getTriangleCount() override;

	void releaseResources() override;

	bool instanced{ false };

private:
	DXContext* context;
	RenderPipeline* renderPipeline;

	Mesh* cube;

	Texture* envCubeMap;

	size_t triangleCount{ 0 };

	GltfData gltfData;

	void construct();
};