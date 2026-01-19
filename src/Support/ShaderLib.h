#pragma once

#include "../Support/WinInclude.h"

#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <unordered_map>
#include <functional>

#include "ComPointer.h"

class ShaderLib {
public:
	ShaderLib() = delete;
	ShaderLib(std::string_view raygenName, std::string_view missName, std::string_view closesthitName);
	inline const void* getBuffer() const { return data; }
	inline size_t getSize() const { return size; }
private:
	void* data = nullptr;
	size_t size = 0;
};