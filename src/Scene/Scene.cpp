#include "Scene.h"

#include "D3D/ResourceManager.h"

Scene::Scene(Camera* p_camera, DXContext* context)
	: camera(p_camera),
	context(context),
	currentRP(),
	currentCP()
{}

RenderPipeline* Scene::getRenderPipeline(UINT index) {
	return renderPipelines[index].get();
}

void Scene::compute() {
	
}

void Scene::draw(D3D12_VIEWPORT& vp) {
	for (std::unique_ptr<Drawable>& drawable : perFrameDrawables) {
		drawable->draw(camera, vp);
	}
}

size_t Scene::getTriangleCount() {
	size_t triangleCount = 0;
	for (std::unique_ptr<Drawable>& drawable : perFrameDrawables) {
		triangleCount += drawable->getTriangleCount();
	}
	return triangleCount;
}

void Scene::releaseResources() {
	for (std::unique_ptr<Drawable>& drawable : perFrameDrawables) {
		drawable->releaseResources();
		drawable.reset();
	}
	perFrameDrawables.clear();

	for (std::unique_ptr<RenderPipeline>& rp : renderPipelines) {
		rp->releaseResources();
		rp.reset();
	}
	renderPipelines.clear();
}