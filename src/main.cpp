#include <iostream>

#include "Support/WinInclude.h"
#include "Support/Window.h"

#include "D3D/DXContext.h"
#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Pipeline/MeshPipeline.h"
#include "D3D/Pipeline/ComputePipeline.h"

#include "Scene/Util/Camera.h"
#include "Scene/PbrScene.h"
#include "Scene/PtScene.h"

#include "Support/ImguiManager.h"

#include "D3D/ResourceManager.h"

int main() {
    DebugLayer debugLayer = DebugLayer();
    DXContext context = DXContext();

    std::unique_ptr<Camera> camera = std::make_unique<Camera>();
    std::unique_ptr<Keyboard> keyboard = std::make_unique<Keyboard>();
    std::unique_ptr<Mouse> mouse = std::make_unique<Mouse>();

	ResourceManager& resourceManager = ResourceManager::get(&context);
	Window& window = Window::get();

    if (!window.init(&context, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        //handle could not initialize window
        std::cout << "could not initialize window\n";
        window.shutdown();
        return false;
    }

    ImguiManager imguiManager{ context };
	ImguiInfo imguiInfo{};

    //set mouse to use the window
    mouse->SetWindow(window.getHWND());

    //initialize scene
    PbrScene pbrScene{camera.get(), &context};
	PtScene ptScene{ camera.get(), &context };

	Scene* currentScene = &pbrScene;
	bool usePtScene = currentScene == &ptScene;
    bool toggleScene = false;
	imguiInfo.triangleCount = currentScene->getTriangleCount();

    //create viewport
    D3D12_VIEWPORT windowViewport = window.getWindowViewport();

    while (!window.getShouldClose()) {
        //update window
        window.update();

        if (window.getShouldResize()) {
            //flush pending buffer operations in swapchain
            context.flush(FRAME_COUNT);
            window.resize();

            camera->updateAspect((float)window.getWidth() / (float)window.getHeight());

            windowViewport = window.getWindowViewport();
        }

        DirectX::Keyboard::State kState = keyboard->GetState();
        DirectX::Mouse::State mState = mouse->GetState();

        mouse->SetMode(mState.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
        camera->kmStateCheck(kState, mState);

		if (imguiInfo.currentScene == CurrentScene::PBR_SCENE && usePtScene) {
			currentScene = &pbrScene;
			usePtScene = false;
			imguiInfo.triangleCount = currentScene->getTriangleCount();
		}
		else if (imguiInfo.currentScene == CurrentScene::PT_SCENE && !usePtScene) {
			currentScene = &ptScene;
			usePtScene = true;
			imguiInfo.triangleCount = currentScene->getTriangleCount();
		}

        Pipeline* pipeline = currentScene->getPrimaryPipeline();

        //begin frame
        window.beginFrame(pipeline->getCommandList());

        //draw scene
        currentScene->draw(windowViewport);

        //render imgui
        Window::get().setCmdListRenderTarget(pipeline->getCommandList());
        imguiManager.render(pipeline->getCommandList(), imguiInfo);
        context.executeCommandList(pipeline->getCommandListID());
        context.resetCommandList(pipeline->getCommandListID());

        //end frame
        window.endFrame(pipeline->getCommandList());

        // Execute command list
        context.executeCommandList(pipeline->getCommandListID());
        window.present();
        context.resetCommandList(pipeline->getCommandListID());
    }

    //scene should release all drawable and pipeline resources
    pbrScene.releaseResources();
	ptScene.releaseResources();

    //release imgui resources
	imguiManager.releaseResources();

    //flush pending buffer operations in swapchain
    context.flush(FRAME_COUNT);

	resourceManager.releaseAllResources();

    window.shutdown();
}
