workspace "FrameDAG"
configurations {"Debug", "Release"}
platforms {"x64"}
language "C++"
cppdialect "C++17"

project "TestApp"
kind "ConsoleApp"
staticruntime "off"

targetdir "bin/%{cfg.buildcfg}"
objdir "bin-int/%{cfg.buildcfg}"

files {"DAG.h", "main.cpp"}

-- Додаємо підтримку потоків (необхідно для Linux/macOS)
filter "system:linux or system:macosx"
links {"pthread"}

filter "configurations:Debug"
runtime "Debug"
symbols "on"

filter "configurations:Release"
runtime "Release"
optimize "on"
