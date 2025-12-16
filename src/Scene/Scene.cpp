#include "Scene.h"

Scene::Scene(Camera* p_camera, DXContext* context)
	:  camera(p_camera),
	objectRP("VertexShader.cso", "PixelShader.cso", *context, CommandListID::OBJECT_RENDER_SOLID_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	objectScene(context, &objectRP),
	currentRP(),
	currentCP()
{}

RenderPipeline* Scene::getObjectPipeline()
{
	return &objectRP;
}

void Scene::compute() {
	//pbmpmScene.compute();
}

void Scene::draw() {
	objectScene.draw(camera);
}

void Scene::releaseResources() {
	objectScene.releaseResources();
}