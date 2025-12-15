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

#include "Scene/Camera.h"
#include "Scene/Scene.h"

#include "ImGUI/ImGUIHelper.h"

static ImGUIDescriptorHeapAllocator imguiHeapAllocator;
static ID3D12DescriptorHeap* imguiSRVHeap = nullptr;

ImGuiIO& initImGUI(DXContext& context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(Window::get().getHWND());

    ImGui_ImplDX12_InitInfo imguiDXInfo;
    imguiDXInfo.CommandQueue = context.getCommandQueue();
    imguiDXInfo.Device = context.getDevice();
    imguiDXInfo.NumFramesInFlight = 2;
    imguiDXInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 64;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (context.getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiSRVHeap)) != S_OK) {
        std::cout << "could not create imgui descriptor heap\n";
        Window::get().shutdown();
    }
    imguiHeapAllocator.Create(context.getDevice(), imguiSRVHeap);

    imguiHeapAllocator.Heap = imguiSRVHeap;
    imguiDXInfo.SrvDescriptorHeap = imguiSRVHeap;
    imguiDXInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return imguiHeapAllocator.Alloc(out_cpu_handle, out_gpu_handle); };
    imguiDXInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return imguiHeapAllocator.Free(cpu_handle, gpu_handle); };

    ImGui_ImplDX12_Init(&imguiDXInfo);

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    
    return io;
}

void setupImGUIWindow(ImGuiIO& io) {

    ImGui::Begin("Renderer Info + Options");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    ImGui::End();
}

void ComputeMouseRay(
    HWND hwnd,
    float ndcX,
    float ndcY,
    const XMMATRIX& projectionMatrix,
    const XMMATRIX& viewMatrix,
    XMFLOAT4& rayOrigin,
    XMFLOAT4& rayDirection
) {
    // Invert the projection and view matrices
    XMMATRIX invProj = XMMatrixInverse(nullptr, projectionMatrix);
    XMMATRIX invView = XMMatrixInverse(nullptr, viewMatrix);

    // Define the mouse's NDC position on the near and far planes
    XMVECTOR ndcNear = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f); // Near plane
    XMVECTOR ndcFar = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);   // Far plane

    // Unproject the NDC points to view space
    XMVECTOR viewNear = XMVector3TransformCoord(ndcNear, invProj);
    XMVECTOR viewFar = XMVector3TransformCoord(ndcFar, invProj);

    // Transform the points from view space to world space
    XMVECTOR worldNear = XMVector3TransformCoord(viewNear, invView);
    XMVECTOR worldFar = XMVector3TransformCoord(viewFar, invView);

    // Calculate the ray origin (camera position) and direction
    rayOrigin = XMFLOAT4(
        XMVectorGetX(worldNear),
        XMVectorGetY(worldNear),
        XMVectorGetZ(worldNear),
        1.0f
    );

    XMVECTOR rayDir = XMVector3Normalize(XMVectorSubtract(worldFar, worldNear));
    rayDirection = XMFLOAT4(
        XMVectorGetX(rayDir),
        XMVectorGetY(rayDir),
        XMVectorGetZ(rayDir),
        0.0f
    );
}