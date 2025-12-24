#pragma once

#include <filesystem>
#include <vector>

#include "Util/Drawable.h"
#include "Util/Loader.h"
#include "Util/Mesh.h"

class ObjectDrawable : public Drawable {
public:
	ObjectDrawable(DXContext* context, RenderPipeline* pipeline);

	void draw(Camera* camera, D3D12_VIEWPORT& vp) override;

	size_t getTriangleCount() override;

	void releaseResources() override;

	bool instanced{ false };

private:
	DXContext* context;
	RenderPipeline* renderPipeline;

	std::vector<Mesh*> meshes;
	std::vector<XMFLOAT4X4> modelMatrices;

	size_t triangleCount{ 0 };

	GltfData gltfData;

    void construct();
};