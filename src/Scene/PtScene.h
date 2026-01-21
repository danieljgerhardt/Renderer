#pragma once

#include "Scene.h"

#include "D3D/Pipeline/RayPipeline.h"

class PtScene : public Scene {
public:
	PtScene(Camera* camera, DXContext* context);

	void updateScene();

	void draw(D3D12_VIEWPORT& vp) override;

	RayPipeline* getRayPipeline() { return rayPipeline.get(); }

private:
	ID3D12Resource* makeAccelStruct(RayPipeline* rayPipeline, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, UINT64* updateScratchSize = nullptr);
	ID3D12Resource* makeBlas(RayPipeline* rayPipeline, VertexBuffer* vertexBuffer, UINT vertexFloats, IndexBuffer* indexBuffer = nullptr, UINT indices = 0);
	ID3D12Resource* makeTlas(RayPipeline* rayPipeline, ID3D12Resource* instances, UINT numInstances, UINT64* updateScratchSize);

	void initTopLevel();

	void updateTransforms();

	D3D12_RAYTRACING_INSTANCE_DESC* instanceData;

	ID3D12Resource* tlas;
	ID3D12Resource* tlasUpdateScratch;

	ID3D12Resource* instances;

	UINT numInstances{ 0 };

	std::unique_ptr<RayPipeline> rayPipeline;

	Texture* renderTarget;
};