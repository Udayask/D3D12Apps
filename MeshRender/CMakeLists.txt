cmake_minimum_required(VERSION 3.5)

set(${CMAKE_CURRENT_SOURCE_DIR}/shaders)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
file(GLOB Shaders ${SHADER_DIR}/*.hlsl)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SourceFiles 
"main.cpp" 
)

add_executable(MeshRender WIN32 ${SourceFiles} ${Shaders})

if (MSVC)
  set_target_properties(MeshRender PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_link_libraries(MeshRender d3d12.lib dxgi.lib D3DCompiler.lib)

add_custom_target(CopyResourcesMR ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/MeshRender/shaders ${PROJECT_BINARY_DIR}/MeshRender/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/MeshRender/textures ${PROJECT_BINARY_DIR}/MeshRender/textures
    COMMENT "Copying resources to binary directory..."
)

add_dependencies(MeshRender CopyResourcesMR)

if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(MeshRender PROPERTIES
                          LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

