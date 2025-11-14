#include "main.h"

int main() {
    //set up DX, window, keyboard mouse
    DebugLayer debugLayer = DebugLayer();
    DXContext context = DXContext();
    std::unique_ptr<Camera> camera = std::make_unique<Camera>();
    std::unique_ptr<Keyboard> keyboard = std::make_unique<Keyboard>();
    std::unique_ptr<Mouse> mouse = std::make_unique<Mouse>();

    if (!Window::get().init(&context, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        //handle could not initialize window
        std::cout << "could not initialize window\n";
        Window::get().shutdown();
        return false;
    }

    //initialize ImGUI
    ImGuiIO& io = initImGUI(context);

    //set mouse to use the window
    mouse->SetWindow(Window::get().getHWND());

    // Get the client area of the window
    RECT rect;
    GetClientRect(Window::get().getHWND(), &rect);
    float clientWidth = static_cast<float>(rect.right - rect.left);
    float clientHeight = static_cast<float>(rect.bottom - rect.top);

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

        auto renderPipeline = scene.getObjectSolidPipeline();

        //begin frame
        Window::get().beginFrame(renderPipeline->getCommandList());

        //create viewport
        D3D12_VIEWPORT vp;
        Window::get().createViewport(vp, renderPipeline->getCommandList());

        Window::get().setRT(renderPipeline->getCommandList());
        Window::get().setViewport(vp, renderPipeline->getCommandList());

		//draw scene
		scene.drawSolidObjects();

        //set up ImGUI for frame
        /*ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();*/

        //draw ImGUI
		/*drawImGUIWindow(pbmpmIterConstants, io,
            scene.getFluidIsovalue(), 
            scene.getFluidKernelScale(), 
            scene.getFluidKernelRadius(),
			scene.getElasticIsovalue(),
			scene.getElasticKernelScale(),
			scene.getElasticKernelRadius(),
			scene.getSandIsovalue(),
			scene.getSandKernelScale(),
			scene.getSandKernelRadius(),
            scene.getViscoIsovalue(),
            scene.getViscoKernelScale(),
            scene.getViscoKernelRadius(),
            scene.getPBMPMSubstepCount(),
            scene.getNumParticles());*/

        //render ImGUI
        /*ImGui::Render();
        if (pbmpmIterConstants.mouseActivation == 1 || !PBMPMScene::constantsEqual(pbmpmIterConstants, pbmpmCurrConstants)) {
            scene.updatePBMPMConstants(pbmpmIterConstants);
            pbmpmCurrConstants = pbmpmIterConstants;
        }*/

       /* renderPipeline->getCommandList()->SetDescriptorHeaps(1, &imguiSRVHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderPipeline->getCommandList());*/

        context.executeCommandList(renderPipeline->getCommandListID());

        // reset the first pipeline so it can end the frame
        context.resetCommandList(renderPipeline->getCommandListID());
        //end frame
        Window::get().endFrame(renderPipeline->getCommandList());
        // Execute command list
		context.executeCommandList(renderPipeline->getCommandListID());

        Window::get().present();
		context.resetCommandList(renderPipeline->getCommandListID());
		
        context.resetCommandList(renderPipeline->getCommandListID());
    }

    // Scene should release all resources, including their pipelines
    scene.releaseResources();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    imguiSRVHeap->Release();

    //flush pending buffer operations in swapchain
    context.flush(FRAME_COUNT);
    Window::get().shutdown();
}
