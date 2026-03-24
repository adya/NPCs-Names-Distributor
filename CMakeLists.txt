cmake_minimum_required(VERSION 3.20)
set(NAME "NPCsNamesDistributor" CACHE STRING "")
set(VERSION 2.6.2 CACHE STRING "")
set(SE_VERSION 1597)
set(AE_VERSION 16)
set(VR_VERSION 1415)

# ---- Options ----

option(COPY_BUILD "Copy the build output to the Skyrim directory." TRUE)
option(BUILD_SKYRIMAE "Build for Skyrim AE" OFF)


# ---- Project ----

project(
	${NAME}
	VERSION ${VERSION}
	LANGUAGES CXX
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	@ONLY
)

# ---- Cache build vars ----

macro(set_from_environment VARIABLE)
	if (NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif ()
endmacro()

macro(find_commonlib_path)
	if (CommonLibName AND NOT ${CommonLibName} STREQUAL "")
		# Check extern
		find_path(CommonLibPath
		include/REL/Relocation.h
		PATHS ${PROJECT_SOURCE_DIR}/extern/${CommonLibName})
		if (${CommonLibPath} STREQUAL "CommonLibPath-NOTFOUND")
			#Check path
			set_from_environment(${CommonLibName}Path)
			set(CommonLibPath ${${CommonLibName}Path})
		endif()
	endif()
endmacro()

set_from_environment(VCPKG_ROOT)
if(BUILD_SKYRIMAE)
	add_compile_definitions(SKYRIM_AE)
	add_compile_definitions(SKYRIM_SUPPORT_AE)
	set(CommonLibName "CommonLibSSE")
	set_from_environment(SkyrimAEPath)
	set(SkyrimPath ${SkyrimAEPath})
	set(SkyrimVersion "Skyrim AE")
	set(VERSION ${VERSION}.${AE_VERSION})
else()
	set(CommonLibName "CommonLibSSE")
	set_from_environment(Skyrim64Path)
	set(SkyrimPath ${Skyrim64Path})
	set(SkyrimVersion "Skyrim SSE")
	set(VERSION ${VERSION}.${SE_VERSION})
endif()
find_commonlib_path()
message(
	STATUS
	"Building ${NAME} ${VERSION} for ${SkyrimVersion} at ${SkyrimPath} with ${CommonLibName} at ${CommonLibPath}."
)

if (DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
else ()
	message(
		WARNING
		"Variable VCPKG_ROOT is not set. Continuing without vcpkg."
	)
endif ()

set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
			"In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
	)
endif()

# ---- Globals ----

# Configure Develop configuration to match Release but with DEV symbol
set(CMAKE_CXX_FLAGS_DEVELOP "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEVELOP "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEVELOP "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEVELOP "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_DEVELOP "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_DEVELOP "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)

# Tell vcpkg to use release libraries for Develop configuration
if(CMAKE_BUILD_TYPE STREQUAL "Develop")
	set(VCPKG_BUILD_TYPE "release" CACHE STRING "")
endif()

add_compile_definitions(
	SKSE_SUPPORT_XBYAK
)

if (MSVC)
	if (NOT ${CMAKE_GENERATOR} STREQUAL "Ninja")
		add_compile_options(
			/MP	# Build with Multiple Processes
		)
	endif ()
endif ()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(Boost_USE_STATIC_LIBS ON)

# ---- Dependencies ----

if (DEFINED CommonLibPath AND NOT ${CommonLibPath} STREQUAL "" AND IS_DIRECTORY ${CommonLibPath})
	add_subdirectory(${CommonLibPath} ${CommonLibName})
else ()
	message(
		FATAL_ERROR
		"Variable ${CommonLibName}Path is not set."
	)
endif()

set(CLIB_UTIL_INCLUDE_DIRS "extern/CLibUtil/include")
set(SKSE_HOOKING_INCLUDE_DIR "extern/SKSEHooking")
find_path(SKSE_MCP_INCLUDE_DIRS "SKSEMCP/SKSEMenuFramework.hpp")

find_package(nlohmann_json 3.12.0 REQUIRED)

# ---- Auto-discover all source and header files ----

file(GLOB_RECURSE ALL_SRC_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

# ---- Auto-discover all public header files ----

file(GLOB_RECURSE PUBLIC_HEADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

# ---- Visual Studio folder grouping ----

# All files go under "Source Files", preserving src/ substructure
foreach(source_file ${ALL_SRC_FILES})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/src" "${source_file}")
    get_filename_component(dir "${rel_path}" PATH)
    string(REPLACE "/" "\\" subfolder "${dir}")
    source_group("Source Files\\${subfolder}" FILES "${source_file}")
endforeach()

# Public headers go under "Header Files", preserving include/ substructure
foreach(header_file ${PUBLIC_HEADER_FILES})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header_file}")
    get_filename_component(dir "${rel_path}" PATH)
    string(REPLACE "/" "\\" subfolder "${dir}")
    if(subfolder)
        source_group("Header Files\\${subfolder}" FILES "${header_file}")
    else()
        source_group("Header Files" FILES "${header_file}")
    endif()
endforeach()

# Remove PCH files from ALL_SRC_FILES
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "src[/\\\\]PCH\\.(cpp|h|hpp)$")

set(PCH_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PCH.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PCH.cpp
)

source_group("Header Files" FILES ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h)

source_group("Precompile Header File" FILES ${PCH_FILES})

source_group("Resource Files" FILES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

# ---- Create DLL ----

add_library(
    ${PROJECT_NAME}
    SHARED
    ${ALL_SRC_FILES}
	${PCH_FILES}
    ${PUBLIC_HEADER_FILES}
    ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    .clang-format
    .editorconfig
)

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
		cxx_std_23
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		_UNICODE
		$<$<CONFIG:Develop>:DEV>
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SKSE_MCP_INCLUDE_DIRS}
        ${CLIB_UTIL_INCLUDE_DIRS}
		${SKSE_HOOKING_INCLUDE_DIR}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		${CommonLibName}::${CommonLibName}
)

target_precompile_headers(
    ${PROJECT_NAME}
    PRIVATE
        src/PCH.h
)

if (MSVC)
	target_compile_options(
		${PROJECT_NAME}
		PRIVATE
			/sdl             # Enable Additional Security Checks
			/utf-8           # Set Source and Executable character sets to UTF-8
			/Zi              # Debug Information Format

			/permissive-     # Standards conformance
			/Zc:preprocessor # Enable preprocessor conformance mode

			/wd4200          # nonstandard extension used : zero-sized array in struct/union

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
			"$<$<CONFIG:DEVELOP>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
			"$<$<CONFIG:DEVELOP>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif ()

# ---- Post build ----

if (COPY_BUILD)
	if (DEFINED SkyrimPath)
		add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${SkyrimPath}/Data/SKSE/Plugins/
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> ${SkyrimPath}/Data/SKSE/Plugins/
		)
	else ()
		message(
			WARNING
			"Variable ${SkyrimPath} is not defined. Skipping post-build copy command."
		)
	endif ()
endif ()
