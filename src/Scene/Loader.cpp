#include "Loader.h"

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Support/WinInclude.h"

#include "D3D/DXContext.h"
#include "D3D/VertexBuffer.h"
#include "D3D/IndexBuffer.h"
#include "D3D/StructuredBuffer.h"

void Loader::loadMeshFromObj(std::string fileLocation, UINT& numTriangles, std::vector<Vertex>& vertices, std::vector<XMFLOAT3>& vertexPositions, std::vector<unsigned int>& indices)
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
            vertexPositions.push_back(XMFLOAT3(v[0], v[1], v[2]));
            vertices.push_back(newVert);
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
        numTriangles++;
        indices.push_back(face[0]);
        indices.push_back(face[1]);
        indices.push_back(face[2]);
    }

    file.close();
}
