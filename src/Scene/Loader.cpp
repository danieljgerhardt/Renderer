#include "Loader.h"

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Support/WinInclude.h"

#include "D3D/DXContext.h"
#include "D3D/Buffer/VertexBuffer.h"
#include "D3D/Buffer/IndexBuffer.h"
#include "D3D/Buffer/StructuredBuffer.h"

#include "D3D/Texture.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "includes/tiny_gltf.h"

void Loader::loadMeshFromObj(std::string fileLocation, MeshData& meshData)
{
    std::ifstream file(fileLocation);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << fileLocation << std::endl;
        return;
    }

    std::string line;
    std::vector<XMFLOAT3> normals;
    std::vector<std::vector<int>> faces; // To store face indices

    // Skip the first few lines (header or unnecessary data)
    for (int i = 0; i < 4; ++i) {
        std::getline(file, line);
    }

    while (std::getline(file, line)) {
        if (line[0] == 'v' && line[1] == ' ') {
            //line is a vertex
            std::string temp = line.substr(2); //skip the "v " prefix
            std::stringstream ss(temp);
            std::vector<float> v;
            float value;

            while (ss >> value) {
                v.push_back(value);
            }
            Vertex newVert;
            newVert.pos = XMFLOAT3(v[0], v[1], v[2]);
            meshData.vertexPositions.push_back(XMFLOAT3(v[0], v[1], v[2]));
            meshData.vertices.push_back(newVert);
        }
        else if (line[0] == 'v' && line[1] == 'n') {
            //line is a normal
            std::string temp = line.substr(3); //skip the "vn " prefix
            std::stringstream ss(temp);
            std::vector<float> v;
            float value;

            while (ss >> value) {
                v.push_back(value);
            }
            normals.push_back(XMFLOAT3(v[0], v[1], v[2]));
        }
        else if (line[0] == 'f') {
            //line is a face
            std::string temp = line.substr(2); //skip the "f " prefix
            std::stringstream ss(temp);
            std::vector<int> vertexIndices;
            std::string s;
            int num1, num2, num3;
            char slash1, slash2;
            //man i love string steams :)
            while (ss >> num1 >> slash1 >> num2 >> slash2 >> num3) {
                //subtract 1 because obj isn't 0 indexed
                vertexIndices.push_back(num1 - 1);
            }

            faces.push_back(vertexIndices);
        }
    }

    for (auto face : faces) {
        meshData.numTriangles++;
        meshData.indices.push_back(face[0]);
        meshData.indices.push_back(face[1]);
        meshData.indices.push_back(face[2]);
    }

    file.close();
}

const float* GetFloatAttrib(
    const tinygltf::Accessor& acc,
    const tinygltf::Model& model,
    size_t index,
    size_t components)
{
    const auto& view = model.bufferViews[acc.bufferView];
    const auto& buffer = model.buffers[view.buffer];

    size_t stride = view.byteStride;
    if (stride == 0)
        stride = components * sizeof(float);

    return reinterpret_cast<const float*>(
        buffer.data.data() +
        view.byteOffset +
        acc.byteOffset +
        index * stride);
}

void Loader::loadDataFromGltf(std::string fileLocation, GltfConstructionData& gltfConstructionData)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, fileLocation);
    //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename); // for binary glTF(.glb)

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF: %s\n", fileLocation.c_str());
    }

    //on success, make a mesh using gltf info
    for (tinygltf::Mesh& mesh : model.meshes) {
		MeshData meshData;
        for (auto& prim : mesh.primitives) {
            const auto& posAcc = model.accessors.at(prim.attributes.at("POSITION"));
            const auto& norAcc = model.accessors.at(prim.attributes.at("NORMAL"));
            const auto& uvAcc = model.accessors.at(prim.attributes.at("TEXCOORD_0"));

            uint32_t baseVertex = static_cast<uint32_t>(meshData.vertices.size());

            for (size_t i = 0; i < posAcc.count; i++)
            {
                const float* pos = GetFloatAttrib(posAcc, model, i, 3);
                const float* nor = GetFloatAttrib(norAcc, model, i, 3);
                const float* uv = GetFloatAttrib(uvAcc, model, i, 2);

                Vertex v{};
                v.pos = { pos[0], pos[1], pos[2] };
                v.nor = { nor[0], nor[1], nor[2] };
                v.uv = { uv[0], uv[1] };

                meshData.vertices.push_back(v);
            }

            const auto& idxAcc = model.accessors.at(prim.indices);
            const auto& idxView = model.bufferViews.at(idxAcc.bufferView);
            const auto& idxBuf = model.buffers.at(idxView.buffer);

            const uint8_t* idxData =
                idxBuf.data.data() +
                idxView.byteOffset +
                idxAcc.byteOffset;

            for (size_t i = 0; i < idxAcc.count; i++)
            {
                uint32_t idx;

                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    idx = reinterpret_cast<const uint16_t*>(idxData)[i];
                else
                    idx = reinterpret_cast<const uint32_t*>(idxData)[i];

                meshData.indices.push_back(baseVertex + idx);
            }
        }
        meshData.numTriangles = static_cast<UINT>(meshData.indices.size() / 3);
		gltfConstructionData.meshDataVector.push_back(meshData);
    }

    for (const auto& material : model.materials) {
        // Diffuse/Base Color
        if (material.values.find("baseColorTexture") != material.values.end()) {
            int textureIndex = material.values.at("baseColorTexture").TextureIndex();
            tinygltf::Image image = model.images[model.textures[textureIndex].source];
            TextureData newTextureData{ UINT(image.width), UINT(image.height), image.image, TextureType::DIFFUSE };
            gltfConstructionData.textureDataVector.push_back(newTextureData);
        }

        // Normal Map
        if (material.additionalValues.find("normalTexture") != material.additionalValues.end()) {
            int textureIndex = material.additionalValues.at("normalTexture").TextureIndex();
            int imageIndex = model.textures[textureIndex].source;
            tinygltf::Image image = model.images[model.textures[textureIndex].source];
            TextureData newTextureData{ UINT(image.width), UINT(image.height), image.image, TextureType::NORMAL };
            gltfConstructionData.textureDataVector.push_back(newTextureData);
        }

        // Metallic-Roughness
        if (material.values.find("metallicRoughnessTexture") != material.values.end()) {
            int textureIndex = material.values.at("metallicRoughnessTexture").TextureIndex();
            tinygltf::Image image = model.images[model.textures[textureIndex].source];
            TextureData newTextureData{ UINT(image.width), UINT(image.height), image.image, TextureType::METALLIC_ROUGHNESS };
            gltfConstructionData.textureDataVector.push_back(newTextureData);
        }

        // Emissive
        if (material.additionalValues.find("emissiveTexture") != material.additionalValues.end()) {
            int textureIndex = material.additionalValues.at("emissiveTexture").TextureIndex();
            tinygltf::Image image = model.images[model.textures[textureIndex].source];
            TextureData newTextureData{ UINT(image.width), UINT(image.height), image.image, TextureType::EMISSIVE };
            gltfConstructionData.textureDataVector.push_back(newTextureData);
        }
    }
}

GltfData Loader::createMeshFromGltf(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix)
{
    //get data from load func
	GltfConstructionData constructionData;
	loadDataFromGltf(fileLocation, constructionData);
	//create and return meshes
	std::vector<Mesh> newMeshes;
    for (auto& meshData : constructionData.meshDataVector) {
        Mesh newMesh = Mesh(fileLocation, context, cmdList, pipeline, modelMatrix, meshData);
		newMeshes.push_back(newMesh);
    }

    std::vector<Texture> newTextures;
    /*for (auto& textureData : constructionData.textureDataVector) {
        newTextures.push_back({ context, pipeline, textureData.width, textureData.height, textureData.imageData, textureData.type });
    }*/
    newTextures.push_back({ context, pipeline, constructionData.textureDataVector[0].width, constructionData.textureDataVector[0].height, constructionData.textureDataVector[0].imageData, constructionData.textureDataVector[0].type});

    return { newMeshes, newTextures };
}
