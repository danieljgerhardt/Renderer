#pragma once

#include "ObjectDrawable.h"
#include "PbrDrawable.h"
#include "CubemapDrawable.h"
#include "CubemapDiffuseConvolution.h"
#include "CubemapGlossyConvolution.h"
#include "EnvironmentMapDrawable.h"

#include "../D3D/Pipeline/RenderPipeline.h"
#include "../D3D/Pipeline/ComputePipeline.h"
#include "../D3D/Pipeline/MeshPipeline.h"

class Scene {
public:
	Scene() = delete;
	Scene(Camera* camera, DXContext* context);

	RenderPipeline* getRenderPipeline(UINT index);

	virtual void compute();

	virtual void draw(D3D12_VIEWPORT& vp);

	virtual size_t getTriangleCount() = 0;

	virtual void releaseResources() = 0;

protected:
	Camera* camera;

	std::vector<std::unique_ptr<RenderPipeline>> renderPipelines;
	std::vector<std::unique_ptr<ComputePipeline>> computePipelines;
	std::vector<std::unique_ptr<Drawable>> perFrameDrawables;

	RenderPipeline* currentRP;
	ComputePipeline* currentCP;

	DXContext* context;
};
