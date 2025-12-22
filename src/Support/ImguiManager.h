#pragma once

#include "Debug/DebugLayer.h"

#include "D3D/DXContext.h"
#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Pipeline/MeshPipeline.h"
#include "D3D/Pipeline/ComputePipeline.h"

#include "Scene/Util/Camera.h"
#include "Scene/Scene.h"

#include "ImGUI/ImGUIHelper.h"

#include "D3D/ResourceManager.h"

//info for imgui rendering
struct ImguiInfo {
    size_t triangleCount{ 0 };
};

class ImguiManager {
public:
    ImguiManager() = delete;
    ImguiManager(DXContext& context);

    void initImgui(DXContext& context);

    void setupImguiWindow(ImguiInfo& imguiInfo);

    ID3D12DescriptorHeap* getImguiSrvHeap();

    void render(ID3D12GraphicsCommandList6* cmdList, ImguiInfo& imguiInfo);

    void releaseResources();

private:
    ImGuiIO* ioPtr;
	DXContext& context;
};