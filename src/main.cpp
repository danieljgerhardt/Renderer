#include <iostream>

#include "Support/WinInclude.h"
#include "Support/Window.h"

#include "D3D/DXContext.h"
#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Pipeline/MeshPipeline.h"
#include "D3D/Pipeline/ComputePipeline.h"

#include "Scene/Util/Camera.h"
#include "Scene/Scene.h"

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
    Scene scene{camera.get(), &context};
	imguiInfo.triangleCount = scene.getTriangleCount();

    while (!window.getShouldClose()) {
        //update window
        window.update();
        if (window.getShouldResize()) {
            //flush pending buffer operations in swapchain
            context.flush(FRAME_COUNT);
            window.resize();
            camera->updateAspect((float)window.getWidth() / (float)window.getHeight());
        }

        DirectX::Keyboard::State kState = keyboard->GetState();
        DirectX::Mouse::State mState = mouse->GetState();
        mouse->SetMode(mState.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
        camera->kmStateCheck(kState, mState);

        RenderPipeline* renderPipeline = scene.getRenderPipeline(0);

        //begin frame
        window.beginFrame(renderPipeline->getCommandList());

        //create viewport
        D3D12_VIEWPORT vp;
        window.createViewport(vp, renderPipeline->getCommandList());

        window.setRT(renderPipeline->getCommandList());
        window.setViewport(vp, renderPipeline->getCommandList());

		//draw scene
		scene.draw();

        //render imgui
		imguiManager.render(renderPipeline->getCommandList(), imguiInfo);
        context.executeCommandList(renderPipeline->getCommandListID());
        context.resetCommandList(renderPipeline->getCommandListID());

        //end frame
        window.endFrame(renderPipeline->getCommandList());

        // Execute command list
		context.executeCommandList(renderPipeline->getCommandListID());
        window.present();
		context.resetCommandList(renderPipeline->getCommandListID());
    }

    //scene should release all drawable and pipeline resources
    scene.releaseResources();

    //release imgui resources
	imguiManager.releaseResources();

    //flush pending buffer operations in swapchain
    context.flush(FRAME_COUNT);

	resourceManager.releaseAllResources();

    window.shutdown();
}
