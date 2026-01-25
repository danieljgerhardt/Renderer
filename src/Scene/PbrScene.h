#pragma once

#include "Scene.h"

class PbrScene : public Scene {
public:
	PbrScene(Camera* camera, DXContext* context);

	void releaseResources() override;

	size_t getTriangleCount() override {
		size_t triangleCount = 0;
		for (std::unique_ptr<Drawable>& drawable : perFrameDrawables) {
			triangleCount += drawable->getTriangleCount();
		}
		return triangleCount;
	}

private:
	std::vector<std::unique_ptr<Drawable>> iblSetupDrawables;
};