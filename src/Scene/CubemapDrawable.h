#pragma once

#include "Scene/Util/Drawable.h"
#include "D3D/Texture.h"
#include "D3D/Buffer/VertexBuffer.h"
#include "D3D/Buffer/IndexBuffer.h"

class CubemapDrawable : public Drawable
{
public:
	CubemapDrawable(DXContext* context, RenderPipeline* pipeline);

	void draw(Camera* camera, D3D12_VIEWPORT& vp);

	size_t getTriangleCount() override;

	void releaseResources() override;

	bool instanced{ false };

private:
	DXContext* context;
	RenderPipeline* renderPipeline;

	Texture* envMap;
	Texture* envCubeMap;

	VertexBuffer* cubeVb;
	D3D12_VERTEX_BUFFER_VIEW cubeVbv;
	IndexBuffer* cubeIb;
	D3D12_INDEX_BUFFER_VIEW cubeIbv;

	std::vector<XMMATRIX> viewMatrices;

	std::array<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>, 6> cubeMapSrvHandles;

	DescriptorHeap* rtvDescriptorHeap;

	void construct();
};
