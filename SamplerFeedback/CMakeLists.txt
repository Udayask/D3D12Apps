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

add_executable(SamplerFeedback WIN32 ${SourceFiles} ${Shaders})

if (MSVC)
  set_target_properties(SamplerFeedback PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(SOURCE ${Shaders} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
endif()

target_link_libraries(SamplerFeedback d3d12.lib dxgi.lib D3DCompiler.lib)

add_custom_target(CopyResourcesSF ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/SamplerFeedback/shaders ${PROJECT_BINARY_DIR}/SamplerFeedback/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/SamplerFeedback/textures ${PROJECT_BINARY_DIR}/SamplerFeedback/textures
    COMMENT "Copying resources to binary directory..."
)

add_dependencies(SamplerFeedback CopyResourcesSF)

if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(SamplerFeedback PROPERTIES
                          LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

