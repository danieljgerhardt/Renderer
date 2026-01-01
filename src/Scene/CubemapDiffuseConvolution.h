#pragma once

#include "Scene/Util/Drawable.h"
#include "D3D/Texture.h"
#include "D3D/Buffer/VertexBuffer.h"
#include "D3D/Buffer/IndexBuffer.h"

class CubemapDiffuseConvolution : public Drawable
{
public:
	CubemapDiffuseConvolution(DXContext* context, RenderPipeline* pipeline, Texture* envCubeMap);

	void draw(Camera* camera, D3D12_VIEWPORT& vp);

	bool drawEveryFrame() override { return false; }

	size_t getTriangleCount() override;

	Texture* getDiffuseConvolution() { return diffuseConvolution; }

	void releaseResources() override;

	bool instanced{ false };

private:
	DXContext* context;
	RenderPipeline* renderPipeline;

	Texture* envCubeMap;
	Texture* diffuseConvolution;

	VertexBuffer* cubeVb;
	D3D12_VERTEX_BUFFER_VIEW cubeVbv;
	IndexBuffer* cubeIb;
	D3D12_INDEX_BUFFER_VIEW cubeIbv;

	std::array<XMMATRIX, 6> viewMatrices;

	std::array<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>, 6> diffuseConvolutionSrvHandles;

	DescriptorHeap* rtvDescriptorHeap;

	void construct();
};
