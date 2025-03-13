cmake_minimum_required(VERSION 3.10)
project(TCoRFix)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

FetchContent_Declare(
	minhook
	GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
	GIT_TAG master
)

FetchContent_MakeAvailable(minhook)

include_directories (${minhook_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} SHARED src/main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".asi")

target_link_libraries(${PROJECT_NAME} minhook)