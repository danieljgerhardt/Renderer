#pragma once

#include "Scene.h"

class PbrScene : public Scene {
public:
	PbrScene(Camera* camera, DXContext* context);
};