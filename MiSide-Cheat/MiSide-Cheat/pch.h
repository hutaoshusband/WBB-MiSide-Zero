// pch.h: Dies ist eine vorkompilierte Headerdatei.
// Die unten aufgeführten Dateien werden nur einmal kompiliert, um die Buildleistung für zukünftige Builds zu verbessern.
#ifndef PCH_H
#define PCH_H

// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// DirectX 11
#include <d3d11.h>
#include <dxgi.h>

// STL
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <thread>
#include <chrono>

// ImGui paths
#define IMGUI_DEFINE_MATH_OPERATORS

#endif //PCH_H
