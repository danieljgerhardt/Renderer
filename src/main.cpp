#include <iostream>

#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "Support/Window.h"
#include "Support/Shader.h"

#include "Debug/DebugLayer.h"

#include "D3D/DXContext.h"
#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Pipeline/MeshPipeline.h"
#include "D3D/Pipeline/ComputePipeline.h"

#include "Scene/Util/Camera.h"
#include "Scene/Scene.h"

#include "ImGUI/ImGUIHelper.h"
#include "Support/ImguiHelper.h"

#include "D3D/ResourceManager.h"

int main() {
    //set up DX, window, keyboard mouse
    DebugLayer debugLayer = DebugLayer();
    DXContext context = DXContext();

    std::unique_ptr<Camera> camera = std::make_unique<Camera>();
    std::unique_ptr<Keyboard> keyboard = std::make_unique<Keyboard>();
    std::unique_ptr<Mouse> mouse = std::make_unique<Mouse>();

	ResourceManager& resourceManager = ResourceManager::get(&context);

    if (!Window::get().init(&context, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        //handle could not initialize window
        std::cout << "could not initialize window\n";
        Window::get().shutdown();
        return false;
    }

    ImguiManager imguiManager{ context };

    //set mouse to use the window
    mouse->SetWindow(Window::get().getHWND());

    //initialize scene
    Scene scene{camera.get(), &context};

    while (!Window::get().getShouldClose()) {
        //update window
        Window::get().update();
        if (Window::get().getShouldResize()) {
            //flush pending buffer operations in swapchain
            context.flush(FRAME_COUNT);
            Window::get().resize();
            camera->updateAspect((float)Window::get().getWidth() / (float)Window::get().getHeight());
        }

        auto kState = keyboard->GetState();
        auto mState = mouse->GetState();
        mouse->SetMode(mState.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
        camera->kmStateCheck(kState, mState);

        auto renderPipeline = scene.getObjectPipeline();

        //begin frame
        Window::get().beginFrame(renderPipeline->getCommandList());

        //create viewport
        D3D12_VIEWPORT vp;
        Window::get().createViewport(vp, renderPipeline->getCommandList());

        Window::get().setRT(renderPipeline->getCommandList());
        Window::get().setViewport(vp, renderPipeline->getCommandList());

		//draw scene
		scene.draw();

        //render imgui
		imguiManager.render(renderPipeline->getCommandList());
        context.executeCommandList(renderPipeline->getCommandListID());
        context.resetCommandList(renderPipeline->getCommandListID());

        //end frame
        Window::get().endFrame(renderPipeline->getCommandList());

        // Execute command list
		context.executeCommandList(renderPipeline->getCommandListID());
        Window::get().present();
		context.resetCommandList(renderPipeline->getCommandListID());
    }

    //scene should release all drawable and pipeline resources
    scene.releaseResources();

    //release imgui resources
	imguiManager.releaseResources();

    //flush pending buffer operations in swapchain
    context.flush(FRAME_COUNT);

	ResourceManager::get(&context).releaseAllResources();

    Window::get().shutdown();
}
