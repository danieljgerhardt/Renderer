#pragma once

#include "ObjectScene.h"
#include "../D3D/Pipeline/RenderPipeline.h"
#include "../D3D/Pipeline/ComputePipeline.h"
#include "../D3D/Pipeline/MeshPipeline.h"

class Scene {
public:
	Scene() = delete;
	Scene(Camera* camera, DXContext* context);

	RenderPipeline* getObjectPipeline();

	void compute();
	void draw();

	void releaseResources();

private:
	Camera* camera;

	RenderPipeline objectRP;
	ObjectScene objectScene;

	RenderPipeline* currentRP;
	ComputePipeline* currentCP;
};
