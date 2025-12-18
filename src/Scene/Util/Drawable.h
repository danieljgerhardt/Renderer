#pragma once

#include "Scene/Util/Camera.h"

#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Pipeline/MeshPipeline.h"

class Drawable {
public:
	Drawable() = delete;
	Drawable(DXContext* context, RenderPipeline* pipeline);
	Drawable(DXContext* context, MeshPipeline* pipeline);

	virtual void constructScene();

	virtual void draw(Camera* camera);

	virtual void releaseResources();

protected:
	DXContext* context;
	RenderPipeline* renderPipeline;
	MeshPipeline* meshPipeline;
};