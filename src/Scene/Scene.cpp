#include "Scene.h"

Scene::Scene(Camera* p_camera, DXContext* context)
	:  camera(p_camera),
	currentRP(),
	currentCP()
{
	renderPipelines.push_back(std::make_unique<RenderPipeline>( "VertexShader.cso", "PixelShader.cso", *context, CommandListID::OBJECT_RENDER_SOLID_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE));

	std::unique_ptr<ObjectDrawable> objScene = std::make_unique<ObjectDrawable>(context, renderPipelines[0].get());
	drawables.push_back(std::move(objScene));

	std::unique_ptr<PbrDrawable> pbrScene = std::make_unique<PbrDrawable>(context, renderPipelines[0].get());
	drawables.push_back(std::move(pbrScene));
}

RenderPipeline* Scene::getObjectPipeline() {
	return renderPipelines[0].get();
}

void Scene::compute() {
	//pbmpmScene.compute();
}

void Scene::draw() {
	for (std::unique_ptr<Drawable>& drawable : drawables) {
		drawable->draw(camera);
	}
}

void Scene::releaseResources() {
	for (std::unique_ptr<Drawable>& drawable : drawables) {
		drawable->releaseResources();
		drawable.reset();
	}
	drawables.clear();
	for (std::unique_ptr<RenderPipeline>& rp : renderPipelines) {
		rp->releaseResources();
		rp.reset();
	}
	renderPipelines.clear();
}