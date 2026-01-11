#pragma once

#include "Scene/Util/Drawable.h"
#include "D3D/Texture.h"
#include "D3D/Buffer/VertexBuffer.h"
#include "D3D/Buffer/IndexBuffer.h"
#include "D3D/Pipeline/ComputePipeline.h"

class CubemapGlossyConvolution : public Drawable
{
public:
	CubemapGlossyConvolution(DXContext* context, RenderPipeline* renderPipeline, ComputePipeline* computePipeline, Texture* envCubeMap);

	void draw(Camera* camera, D3D12_VIEWPORT& vp);

	bool drawEveryFrame() override { return false; }

	Texture* getGlossyConvolution() { return glossyConvolution; }

	size_t getTriangleCount() override;

	void releaseResources() override;

	bool instanced{ false };

private:
	DXContext* context;
	RenderPipeline* renderPipeline;
	ComputePipeline* computePipeline;

	Texture* envCubeMap;
	Texture* glossyConvolution;

	VertexBuffer* cubeVb;
	D3D12_VERTEX_BUFFER_VIEW cubeVbv;
	IndexBuffer* cubeIb;
	D3D12_INDEX_BUFFER_VIEW cubeIbv;

	std::array<XMMATRIX, 6> viewMatrices;

	std::array<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>, 6> glossyConvolutionSrvHandles;

	DescriptorHeap* rtvDescriptorHeap;

	bool mipMapsGenerated{ false };

	void construct();
};
