#include "Scene.h"

Scene::Scene(Camera* p_camera, DXContext* context)
	:  camera(p_camera),
	objectRPSolid("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::OBJECT_RENDER_SOLID_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	objectSceneSolid(context, &objectRPSolid),
	currentRP(),
	currentCP()
{}

RenderPipeline* Scene::getObjectSolidPipeline()
{
	return &objectRPSolid;
}

void Scene::compute() {
	//pbmpmScene.compute();
}

void Scene::drawSolidObjects() {
	objectSceneSolid.draw(camera);
}

void Scene::releaseResources() {
	objectSceneSolid.releaseResources();
}