#pragma once

#include "ObjectDrawable.h"
#include "PbrDrawable.h"
#include "CubemapDrawable.h"

#include "../D3D/Pipeline/RenderPipeline.h"
#include "../D3D/Pipeline/ComputePipeline.h"
#include "../D3D/Pipeline/MeshPipeline.h"

class Scene {
public:
	Scene() = delete;
	Scene(Camera* camera, DXContext* context);

	RenderPipeline* getRenderPipeline(UINT index);

	void compute();

	void draw(D3D12_VIEWPORT& vp);

	size_t getTriangleCount();

	void releaseResources();

private:
	Camera* camera;

	std::vector<std::unique_ptr<RenderPipeline>> renderPipelines;
	std::vector<std::unique_ptr<Drawable>> drawables;

	RenderPipeline* currentRP;
	ComputePipeline* currentCP;
};
