#pragma once

#include <filesystem>
#include <vector>

#include "Util/Drawable.h"
#include "D3D/Buffer/IndexBuffer.h"
#include "D3D/Buffer/VertexBuffer.h"
#include "D3D/Texture.h"

class EnvironmentMapDrawable : public Drawable {
public:
	EnvironmentMapDrawable(DXContext* context, RenderPipeline* pipeline, Texture* envCubeMap);

	void draw(Camera* camera, D3D12_VIEWPORT& vp) override;

	bool drawEveryFrame() override { return true; }

	size_t getTriangleCount() override;

	void releaseResources() override;

	bool instanced{ false };

private:
	DXContext* context;
	RenderPipeline* renderPipeline;

	VertexBuffer* cubeVb;
	D3D12_VERTEX_BUFFER_VIEW cubeVbv;

	IndexBuffer* cubeIb;
	D3D12_INDEX_BUFFER_VIEW cubeIbv;

	Texture* envCubeMap;

	size_t triangleCount{ 0 };

	void construct();
};