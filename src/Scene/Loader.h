#pragma once

#include <string>
#include "Vertex.h"

class Loader {
public:
	static void loadMeshFromObj(std::string fileLocation, UINT& numTriangles, std::vector<Vertex>& vertices, std::vector<XMFLOAT3>& vertexPositions, std::vector<unsigned int>& indices);
};
