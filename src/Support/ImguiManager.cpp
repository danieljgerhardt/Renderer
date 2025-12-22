#include "ImguiManager.h"

static ID3D12DescriptorHeap* imguiSrvHeapPtr = nullptr;
static DescriptorHeap* imguiSrvHeap = nullptr;

ImguiManager::ImguiManager(DXContext& context) : context(context) {
    initImgui(context);
}

void ImguiManager::initImgui(DXContext& context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ioPtr = &ImGui::GetIO();
	ImGuiIO& io = *ioPtr;
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(Window::get().getHWND());

    ImGui_ImplDX12_InitInfo imguiDXInfo;
    imguiDXInfo.CommandQueue = context.getCommandQueue();
    imguiDXInfo.Device = context.getDevice();
    imguiDXInfo.NumFramesInFlight = FRAME_COUNT;
    imguiDXInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    ResourceHandle imguiHeapHandle = ResourceManager::get(&context).createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    imguiSrvHeap = ResourceManager::get(&context).getDescriptorHeap(imguiHeapHandle);
    imguiSrvHeapPtr = imguiSrvHeap->getAddress();

    imguiDXInfo.SrvDescriptorHeap = imguiSrvHeapPtr;
    imguiDXInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
        imguiSrvHeap->allocate(out_cpu_handle, out_gpu_handle);
        };
    imguiDXInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {
        imguiSrvHeap->free(cpu_handle, gpu_handle);
        };

    ImGui_ImplDX12_Init(&imguiDXInfo);

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
}

void ImguiManager::setupImguiWindow(ImguiInfo& imguiInfo) {
    ImGui::Begin("Renderer Info + Options");

    ImGui::Text("%.1f FPS", ioPtr->Framerate);

	ImGui::Text("Triangle Count: %zu", imguiInfo.triangleCount);

    ImGui::End();
}

ID3D12DescriptorHeap* ImguiManager::getImguiSrvHeap() {
    return imguiSrvHeapPtr;
}

void ImguiManager::render(ID3D12GraphicsCommandList6* cmdList, ImguiInfo& imguiInfo) {
    //set up ImGUI for frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //draw ImGUI
    setupImguiWindow(imguiInfo);

    //render ImGUI
    ImGui::Render();

    ID3D12DescriptorHeap* imguiHeapPtr = getImguiSrvHeap();
    cmdList->SetDescriptorHeaps(1, &imguiHeapPtr);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImguiManager::releaseResources() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
