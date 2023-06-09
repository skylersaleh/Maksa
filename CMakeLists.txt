cmake_minimum_required(VERSION 3.15) # Needed to set MSVC Runtime type
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)

if(APPLE AND (NOT IOS) AND (NOT ANDROID))
  set(MACOS 1)
endif()
if((MACOS OR IOS) AND (NOT EMSCRIPTEN))
  project(Maksa OBJC C CXX)
  if(IOS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -x objective-c")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")
  endif()
else()
  project(Maksa C CXX)
endif()


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(LINK_LIBS "")
if (EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=0 -s ELIMINATE\_DUPLICATE\_FUNCTIONS=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2  -s ENVIRONMENT=web -s ASSERTIONS=0 -s WASM=1 -DPLATFORM_WEB --shell-file ${PROJECT_SOURCE_DIR}/src/shell.html --preload-file ${PROJECT_SOURCE_DIR}/assets/@/assets/ -s USE_CLOSURE_COMPILER=0 ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=0 -s ELIMINATE\_DUPLICATE\_FUNCTIONS=1  -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s ENVIRONMENT=web -s ASSERTIONS=0 -s WASM=1 -DPLATFORM_WEB --shell-file ${PROJECT_SOURCE_DIR}/src/shell.html --preload-file ${PROJECT_SOURCE_DIR}/assets/@/assets/ -s USE_CLOSURE_COMPILER=0 ")
endif ()
if (MSVC)
    # Perform extremely aggressive optimization on Release builds:
    # Flags on Visual Studio 2010 and newer:
    # Runtime library: Multi-threaded (/MT) as opposed to default 'Multi-threaded DLL' - static runtime library allows better LTCG inlining opportunities.
    # Optimization: Full Optimization (/Ox)
    # Inline Function Expansion: Any Suitable (/Ob2)
    # Enable Intrinsic Functions: Yes (/Oi)
    # Favor Size Or Speed: Favor fast code (/Ot)
    # Enable Fiber-Safe Optimizations: Yes (/GT)
    # Enable String Pooling: Yes (/GF)
    # Buffer Security Check: No (/GS-)
    # Floating Point Model: Fast (/fp:fast)
    # Enable Floating Point Exceptions: No (/fp:except-)
    # Build with Multiple Processes (/MP)
    # No secure development lifecycle features
    # Exception handling model: Catch C++ exceptions only, assume that "extern C" functions will never throw a C++ exception.
    # Disable all forms of MSVC debug iterator checking in new and old Visual Studios.
    # Causes the application to use the multithread, static version of the run-time library. 
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT /Ox /Ob2 /Oi /Ot /GT /GF /GS- /fp:fast /fp:except- /MP /sdl- /EHsc /D_SECURE_SCL=0 /D_SCL_SECURE_NO_WARNINGS /D_ITERATOR_DEBUG_LEVEL=0 /D_HAS_ITERATOR_DEBUGGING=0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT /Ox /Ob2 /Oi /Ot /GT /GF /GS- /fp:fast /fp:except- /MP /sdl- /EHsc /D_SECURE_SCL=0 /D_SCL_SECURE_NO_WARNINGS /D_ITERATOR_DEBUG_LEVEL=0 /D_HAS_ITERATOR_DEBUGGING=0 ")

endif()

if( (NOT ANDROID))
    set(USE_SDL 1)
    add_definitions(-DUSE_SDL=1)
endif()

if (NOT EMSCRIPTEN)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -O3 ")
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -g -O3 ")
  set(UNICODE_GUI 1)
else () 
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -O3 -DNDEBUG") 
endif ()
if(IOS)
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum iOS deployment version" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO" FORCE)
endif()
if(WIN32)
  message("Building for Windows")
  add_definitions(-DPLATFORM_WINDOWS=1)
endif()
if(ANDROID)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}")
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS}") 
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
if(NOT IOS)
  find_package(OpenGL REQUIRED)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Linux -pthread shenanigans
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    find_package(ALSA REQUIRED)
    set(PLATFORM_LINUX TRUE)
endif()

#=== LIBRARY: cimgui + Dear ImGui
add_library(cimgui STATIC 
    src/cimgui/cimgui.cpp 
    src/cimgui/cimgui.h 
    src/cimgui/imgui/imgui.cpp
    src/cimgui/imgui/imgui.h
    src/cimgui/imgui/imgui_widgets.cpp 
    src/cimgui/imgui/imgui_draw.cpp 
    src/cimgui/imgui/imgui_tables.cpp
    src/cimgui/imgui/imgui_demo.cpp
    )
set_property(TARGET cimgui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")

target_include_directories(cimgui INTERFACE src/cimgui)
if(USE_SDL)
    message("Build with SDL!")

    #=== LIBRARY: SDL2
    set(SDL_SHARED OFF CACHE BOOL "Build Shared" FORCE)
    set(SDL_STATIC ON CACHE BOOL "Build Static" FORCE)
    set(SDL_FORCE_STATIC_VCRT ON CACHE BOOL "USE LIBC" FORCE)
    set(SDL_LIBC ON CACHE BOOL "USE LIBC" FORCE)    
    set(SDL_STATIC_PIC ON CACHE BOOL "Build Static PIC" FORCE)
    # Allow some projects to be built conditionally.
    set(SDL2_DISABLE_SDL2MAIN  ON CACHE BOOL "Disable building/installation of SDL2main" FORCE)
    set(SDL2_DISABLE_INSTALL    ON CACHE BOOL "Disable installation of SDL2" FORCE)
    set(SDL2_DISABLE_UNINSTALL  ON CACHE BOOL "Disable uninstallation of SDL2" FORCE)    

    set(SDL_LIBC               ON CACHE BOOL "Use the system C library" ${OPT_DEF_LIBC} FORCE)
    set(SDL_ATOMIC OFF CACHE BOOL "SDL Video support" FORCE)
    set(SDL_AUDIO ON CACHE BOOL "SDL Video support" FORCE)
    set(SDL_FILE OFF CACHE BOOL "SDL Video support" FORCE)
    set(SDL_THREAD OFF CACHE BOOL "SDL Video support" FORCE)
    set(SDL_RENDER OFF CACHE BOOL "SDL Video support" FORCE)
    set(SDL_LOCALE OFF CACHE BOOL "SDL Video support" FORCE)    

    add_definitions(-DSDL_LEAN_AND_MEAN=1)
    add_subdirectory(src/SDL2)
    set_property(TARGET SDL2-static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so taht you can directly open it.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sEXPORTED_FUNCTIONS=[_main] -sEXPORTED_RUNTIME_METHODS=[ccall,autoResumeAudioContext,dynCall]  ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sEXPORTED_FUNCTIONS=[_main] -sEXPORTED_RUNTIME_METHODS=[ccall,autoResumeAudioContext,dynCall] ")
endif ()
find_package(OpenGL REQUIRED)

if(ANDROID)
    add_definitions(-DPLATFORM_ANDROID)
    set(LINK_LIBS ${LINK_LIBS}  GLESv3 EGL OpenSLES log android)
    message("Android Build!")
endif()

find_package(Git)

if(GIT_EXECUTABLE)
  # Generate a git-describe version string from Git repository tags
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(NOT GIT_DESCRIBE_ERROR_CODE)
    set(GIT_COMMIT_HASH ${GIT_DESCRIBE})
  endif()
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(NOT GIT_DESCRIBE_ERROR_CODE)
    set(GIT_BRANCH ${GIT_DESCRIBE})
  endif()
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(NOT GIT_DESCRIBE_ERROR_CODE)
    set(GIT_TAG ${GIT_DESCRIBE})
  endif()
endif()

# Final fallback: Just use a bogus version string that is semantically older
# than anything else and spit out a warning to the developer.
if(NOT DEFINED GIT_COMMIT_HASH)
  set(GIT_COMMIT_HASH "(Modified Local Build)")
  message(WARNING "Failed to determine GIT_COMMIT_HASH from Git tags. Using default version \"${GIT_COMMIT_HASH}\".")
endif()
add_definitions(-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\")
add_definitions(-DGIT_BRANCH=\"${GIT_BRANCH}\")
add_definitions(-DGIT_TAG=\"${GIT_TAG}\")

set(MAKSA_SRC src/main.c src/stb.c src/miniz.c)
include_directories(src/glad/include)

if(USE_SDL)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

if(IOS)
    set(MAKSA_SRC ${MAKSA_SRC} src/ios_support.m)
    add_definitions(-DPLATFORM_IOS)
endif()

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icons/appicon.rc")
    add_executable(${PROJECT_NAME} ${MAKSA_SRC} ${APP_ICON_RESOURCE_WINDOWS})
elseif(MACOS OR IOS)
    if(MACOS)
      add_definitions(-DPLATFORM_MAC=1)
      set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version" FORCE)
    endif()

    set(MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    set(APP_ICON src/resources/icons/AppIcon.icns)
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(${PROJECT_NAME} ${MAKSA_SRC} ${APP_ICON})
    if(IOS)
        set_property(TARGET ${PROJECT_NAME} PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "SkyEmu")
        set_property(TARGET ${PROJECT_NAME} PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/ios-info.plist.in")
        set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
    endif()
elseif(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${MAKSA_SRC})
else()
    add_executable(${PROJECT_NAME} ${MAKSA_SRC})
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-s TOTAL_MEMORY=256MB -lidbfs.js -s ELIMINATE\_DUPLICATE\_FUNCTIONS=1 -flto -s USE_CLOSURE_COMPILER=0 --closure 0 ")
endif ()
set(LINK_LIBS ${LINK_LIBS} ${OPENGL_LIBRARY} ${ALSA_LIBRARIES})


if(USE_SDL)
  set(LINK_LIBS ${LINK_LIBS} SDL2-static)
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
elseif(EMSCRIPTEN)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icons/favicon_package
                ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()

ADD_CUSTOM_TARGET(do_always ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)

