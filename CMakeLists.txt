# CMakeList.txt: MyRender 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MyRender")

# 在你的CMakeLists.txt文件中，全局添加头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/nlohmann
    ${CMAKE_SOURCE_DIR}/SDL/include
    ${CMAKE_SOURCE_DIR}/core
	${CMAKE_SOURCE_DIR}/gpu
    ${CMAKE_SOURCE_DIR}/base
)

link_directories(${CMAKE_SOURCE_DIR}/SDL/lib)


#set(CORE_FILES core/Vector.hpp )
file(GLOB CORE_FILES "core/*.h" "core/*.hpp" "core/*.cpp")
file(GLOB BASE_FILSE "base/*.hpp" "base/*.h" "base/*.cpp" "base/*.c")
file(GLOB GPU_FILSE "gpu/*.hpp" "gpu/*.h" "gpu/*.cpp")
set(SOURCES MyRender.cpp MyRender.h)
MESSAGE( STATUS "this CORE_FILES key = ${CORE_FILES}.")
MESSAGE( STATUS "this HELP_FILSE key = ${HELP_FILSE}.")
MESSAGE( STATUS "this GPU_FILSE key = ${GPU_FILSE}.")

source_group(main FILES ${SOURCES})
source_group(base FILES ${BASE_FILSE})
source_group(core FILES ${CORE_FILES})
source_group(gpu FILES ${GPU_FILSE})



# 将源代码添加到此项目的可执行文件。
add_executable (MyRender ${SOURCES} ${CORE_FILES} ${GPU_FILSE} ${BASE_FILSE})
target_link_libraries(MyRender SDL3-static)

file(GLOB TEST_FILSE "test/*.hpp" "test/*.h" "test/*.cpp" "test/*.c")
set(SOURCES_TEST test.cpp test.h)
source_group(test_main FILES ${SOURCES_TEST})
source_group(test FILES ${TEST_FILSE})
add_executable (test ${SOURCES_TEST} ${CORE_FILES} ${GPU_FILSE} ${BASE_FILSE} ${TEST_FILSE})


if(CMAKE_SYSTEM_NAME MATCHES "Windows")
target_link_libraries(MyRender Setupapi.lib)
target_link_libraries(MyRender Winmm.lib)
target_link_libraries(MyRender Imm32.lib)
target_link_libraries(MyRender Version.lib)
endif()


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MyRender PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
