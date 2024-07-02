cmake_minimum_required(VERSION 3.5)

set(DIRECTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/directx)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SourceFiles 
"Main.cpp" "ImageManager.cpp" "QueryHelper.cpp" "TemporaryResourceTransition.cpp"
)

add_executable(MeshHlk WIN32 ${SourceFiles})

target_include_directories(MeshHlk PRIVATE ${DIRECTX_DIR})

if (MSVC)
  set_target_properties(MeshHlk PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_link_libraries(MeshHlk d3d12.lib dxgi.lib D3DCompiler.lib)

if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(MeshHlk PROPERTIES
                          LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()
