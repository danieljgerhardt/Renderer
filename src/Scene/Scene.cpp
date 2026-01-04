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
	PipelineFormat standardPipelineFormat{.depthMode = DepthMode::STANDARD, .renderTargetFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM};
	renderPipelines.push_back(std::make_unique<RenderPipeline>( "VertexShader.cso", "PixelShader.cso", *context, CommandListID::OBJECT_RENDER_SOLID_ID,
		renderHeap, standardPipelineFormat));

	//pbr rendering
	renderPipelines.push_back(std::make_unique<RenderPipeline>("PbrVertexShader.cso", "PbrPixelShader.cso", *context, CommandListID::PBR_RENDER_ID,
		renderHeap, standardPipelineFormat));

	//cubemap uv conversion
	PipelineFormat cubemapPipelineFormat{ .depthMode = DepthMode::DISABLED, .renderTargetFormat = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT };
	renderPipelines.push_back(std::make_unique<RenderPipeline>("CubemapVert.cso", "CubemapUvConversionPixel.cso", *context, CommandListID::CUBEMAP_UV_CONVERSION_ID,
		renderHeap, cubemapPipelineFormat));

	//diffuse convolution
	PipelineFormat depthDisabledFormat{ .depthMode = DepthMode::DISABLED, .renderTargetFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM };
	renderPipelines.push_back(std::make_unique<RenderPipeline>("CubemapVert.cso", "DiffuseConvolutionPixel.cso", *context, CommandListID::CUBEMAP_DIFFUSE_CONVOLUTION_ID,
		renderHeap, depthDisabledFormat));

	//glossy convolution
	renderPipelines.push_back(std::make_unique<RenderPipeline>("CubemapVert.cso", "GlossyConvolutionPixel.cso", *context, CommandListID::CUBEMAP_GLOSSY_CONVOLUTION_ID,
		renderHeap, depthDisabledFormat));

	//render environment map
	PipelineFormat envMapPipelineFormat{ .depthMode = DepthMode::ENVIRONMENT_MAP, .renderTargetFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM };
	renderPipelines.push_back(std::make_unique<RenderPipeline>("EnvMapVert.cso", "EnvMapPixel.cso", *context, CommandListID::ENV_MAP_ID,
		renderHeap, envMapPipelineFormat));

	std::unique_ptr<ObjectDrawable> objScene = std::make_unique<ObjectDrawable>(context, renderPipelines[0].get());
	drawables.push_back(std::move(objScene));

	std::unique_ptr<PbrDrawable> pbrScene = std::make_unique<PbrDrawable>(context, renderPipelines[1].get());
	drawables.push_back(std::move(pbrScene));

	std::unique_ptr<CubemapDrawable> cubemapUvConversionScene = std::make_unique<CubemapDrawable>(context, renderPipelines[2].get());
	Texture* envCubeMap = cubemapUvConversionScene->getEnvCubeMap();
	CubemapDrawable* cubemapPtr = cubemapUvConversionScene.get();
	drawables.push_back(std::move(cubemapUvConversionScene));

	std::unique_ptr<CubemapDiffuseConvolution> cubemapDiffuseConvolutionScene = std::make_unique<CubemapDiffuseConvolution>(context, renderPipelines[3].get(), envCubeMap);
	drawables.push_back(std::move(cubemapDiffuseConvolutionScene));

	std::unique_ptr<CubemapGlossyConvolution> cubemapGlossyConvolutionScene = std::make_unique<CubemapGlossyConvolution>(context, renderPipelines[4].get(), envCubeMap);
	drawables.push_back(std::move(cubemapGlossyConvolutionScene));

	std::unique_ptr<EnvironmentMapDrawable> environmentMapScene = std::make_unique<EnvironmentMapDrawable>(context, renderPipelines[5].get(), envCubeMap);
	drawables.push_back(std::move(environmentMapScene));

	for (std::unique_ptr<Drawable>& drawable : drawables) {
		D3D12_VIEWPORT tempVp{};
		if (!drawable->drawEveryFrame()) {
			drawable->draw(camera, tempVp);
		}
	}

	cubemapPtr->getEnvCubeMap()->makeSrv(context, renderPipelines[2].get(), D3D12_SRV_DIMENSION_TEXTURECUBE);
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