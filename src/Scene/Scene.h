#pragma once

#include "ObjectScene.h"
#include "../D3D/Pipeline/RenderPipeline.h"
#include "../D3D/Pipeline/ComputePipeline.h"
#include "../D3D/Pipeline/MeshPipeline.h"

class Scene {
public:
	Scene() = delete;
	Scene(Camera* camera, DXContext* context);

	RenderPipeline* getObjectSolidPipeline();

	void compute();
	void drawSolidObjects();

	void releaseResources();

private:
	Camera* camera;

	RenderPipeline objectRPSolid;
	ObjectScene objectSceneSolid;

	RenderPipeline* currentRP;
	ComputePipeline* currentCP;
};
