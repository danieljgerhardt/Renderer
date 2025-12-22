#pragma once

#include "Scene/Util/Camera.h"

#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Pipeline/MeshPipeline.h"

class Drawable {
public:
	Drawable(){};

	virtual void construct() = 0;

	virtual void draw(Camera* camera) = 0;

	virtual void releaseResources() = 0;

	virtual size_t getTriangleCount() = 0;
};