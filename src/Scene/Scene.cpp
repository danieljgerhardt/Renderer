#include "Scene.h"

#include "D3D/ResourceManager.h"

Scene::Scene(Camera* p_camera, DXContext* context)
	:  camera(p_camera),
	currentRP(),
	currentCP()
{
	ResourceManager& rm = ResourceManager::get(context);
	ResourceHandle renderHeapHandle = rm.createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	DescriptorHeap* renderHeap = rm.getDescriptorHeap(renderHeapHandle);

	//solid object rendering
	renderPipelines.push_back(std::make_unique<RenderPipeline>( "VertexShader.cso", "PixelShader.cso", *context, CommandListID::OBJECT_RENDER_SOLID_ID,
		renderHeap, DepthMode::STANDARD));
	RenderPipeline* objectPipe = renderPipelines.back().get();

	//pbr rendering
	renderPipelines.push_back(std::make_unique<RenderPipeline>("PbrVertexShader.cso", "PbrPixelShader.cso", *context, CommandListID::PBR_RENDER_ID,
		renderHeap, DepthMode::STANDARD));
	RenderPipeline* pbrPipe = renderPipelines.back().get();

	//cubemap uv conversion
	renderPipelines.push_back(std::make_unique<RenderPipeline>("CubemapVert.cso", "CubemapUvConversionPixel.cso", *context, CommandListID::CUBEMAP_UV_CONVERSION_ID,
		renderHeap, DepthMode::DISABLED, DXGI_FORMAT_R32G32B32A32_FLOAT));
	RenderPipeline* cubemapUvConversionPipe = renderPipelines.back().get();

	//diffuse convolution
	renderPipelines.push_back(std::make_unique<RenderPipeline>("CubemapVert.cso", "DiffuseConvolutionPixel.cso", *context, CommandListID::CUBEMAP_DIFFUSE_CONVOLUTION_ID,
		renderHeap, DepthMode::DISABLED));
	RenderPipeline* cubemapDiffuseConvolutionPipe = renderPipelines.back().get();

	//glossy convolution
	renderPipelines.push_back(std::make_unique<RenderPipeline>("CubemapVert.cso", "GlossyConvolutionPixel.cso", *context, CommandListID::CUBEMAP_GLOSSY_CONVOLUTION_ID,
		renderHeap, DepthMode::DISABLED));
	RenderPipeline* cubemapGlossyConvolutionPipe = renderPipelines.back().get();

	//render environment map
	renderPipelines.push_back(std::make_unique<RenderPipeline>("EnvMapVert.cso", "EnvMapPixel.cso", *context, CommandListID::ENV_MAP_ID,
		renderHeap, DepthMode::ENVIRONMENT_MAP));
	RenderPipeline* envMapPipe = renderPipelines.back().get();

	std::unique_ptr<CubemapDrawable> cubemapUvConversionScene = std::make_unique<CubemapDrawable>(context, cubemapUvConversionPipe);
	CubemapDrawable* cubemapPtr = cubemapUvConversionScene.get();
	D3D12_VIEWPORT tempVp{};
	cubemapPtr->draw(camera, tempVp);
	Texture* envCubeMap = cubemapUvConversionScene->getEnvCubeMap();
	drawables.push_back(std::move(cubemapUvConversionScene));

	std::unique_ptr<CubemapDiffuseConvolution> cubemapDiffuseConvolutionScene = std::make_unique<CubemapDiffuseConvolution>(context, cubemapDiffuseConvolutionPipe, envCubeMap);
	CubemapDiffuseConvolution* cubemapDiffuseConvolutionPtr = cubemapDiffuseConvolutionScene.get();
	drawables.push_back(std::move(cubemapDiffuseConvolutionScene));

	std::unique_ptr<CubemapGlossyConvolution> cubemapGlossyConvolutionScene = std::make_unique<CubemapGlossyConvolution>(context, cubemapGlossyConvolutionPipe, envCubeMap);
	CubemapGlossyConvolution* cubemapGlossyConvolutionPtr = cubemapGlossyConvolutionScene.get();
	drawables.push_back(std::move(cubemapGlossyConvolutionScene));

	std::unique_ptr<EnvironmentMapDrawable> environmentMapScene = std::make_unique<EnvironmentMapDrawable>(context, envMapPipe, envCubeMap);
	drawables.push_back(std::move(environmentMapScene));

	cubemapGlossyConvolutionPtr->draw(camera, tempVp);
	cubemapDiffuseConvolutionPtr->draw(camera, tempVp);

	std::unique_ptr<ObjectDrawable> objScene = std::make_unique<ObjectDrawable>(context, objectPipe);
	drawables.push_back(std::move(objScene));

	std::unique_ptr<PbrDrawable> pbrScene = std::make_unique<PbrDrawable>(context, pbrPipe, 
		cubemapDiffuseConvolutionPtr->getDiffuseConvolution(), cubemapGlossyConvolutionPtr->getGlossyConvolution());
	drawables.push_back(std::move(pbrScene));
}

RenderPipeline* Scene::getRenderPipeline(UINT index) {
	return renderPipelines[index].get();
}

void Scene::compute() {
	//pbmpmScene.compute();
}

void Scene::draw(D3D12_VIEWPORT& vp) {
	for (std::unique_ptr<Drawable>& drawable : drawables) {
		if (!drawable->drawEveryFrame()) {
			continue;
		}
		drawable->draw(camera, vp);
	}
	
}

size_t Scene::getTriangleCount() {
	size_t triangleCount = 0;
	for (std::unique_ptr<Drawable>& drawable : drawables) {
		triangleCount += drawable->getTriangleCount();
	}
	return triangleCount;
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