
cmake_minimum_required( VERSION 3.5 )
project( Quake2)

message( "build type: ${CMAKE_BUILD_TYPE}" )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set(QUAKE2_CORE "Quake2Core")
set(GAME_API "game")
set(NET_QUAKE2 "NetQuake2")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}" ${CMAKE_MODULE_PATH})


find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
    message(STATUS "[SDL2_INCLUDES: ${SDL2_INCLUDE_DIRS}]")
    message(STATUS "[SDL2_LIBRARIES: ${SDL2_LIBRARIES}]")
endif()
find_package(OpenGLES2 REQUIRED)
#find_package(GLEW REQUIRED)


set( UNIX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/backends/unix )
set( BACKENDS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/backends )
set( SDL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/sdl )
set( CLIENT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/client )
set( MENU_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/client/menu )
set( REFRESH_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/client/refresh )
set( REFRESH_FILES_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/client/refresh/files )
set( SOUND_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/client/sound )
set( COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/common )
set( SHARED_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/shared )
set( COMMON_SHARED_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/common/shared )
set( UNZIP_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/unzip )
set( COMMON_UNZIP_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/common/unzip )
set( ZLIB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Engine/External/Sources/zlib )
set( SERVER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/server )
set( OPENGLES_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Engine/Sources/Compatibility/OpenGLES )
set( BACKENDS_SDL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/backends/sdl )

set( INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)

set( GAME_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/game )
set( MONSTER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/game/monster )
set( PLAYER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/game/player )
set( SAVE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/game/savegame )


set( COMPABILITY_SDL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Engine/Sources/Compatibility/SDL )
set( COMPABILITY_NML_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Engine/Sources/Compatibility/netML )
set( CLIENT_SDL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/client/refresh/sdl )
set( CLIENT_NML_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources/client/refresh/nml )

file(GLOB COMMON_SOURCE_FILES
    "${UNIX_SOURCE_DIR}/*.c"
    "${BACKENDS_SOURCE_DIR}/*.c"
    "${BACKENDS_SDL_SOURCE_DIR}/*.c"
    "${SDL_SOURCE_DIR}/*.c"
    "${CLIENT_SOURCE_DIR}/*.c"
    "${MENU_SOURCE_DIR}/*.c"
    "${REFRESH_SOURCE_DIR}/*.c"
    "${REFRESH_FILES_SOURCE_DIR}/*.c"
    "${SOUND_SOURCE_DIR}/*.c"
    "${COMMON_SOURCE_DIR}/*.c"
    "${COMMON_UNZIP_SOURCE_DIR}/*.c"
    "${SHARED_SOURCE_DIR}/*.c"
    "${UNZIP_SOURCE_DIR}/*.c"
    "${SERVER_SOURCE_DIR}/*.c"
    "${OPENGLES_SOURCE_DIR}/*.c"
    "${COMMON_SHARED_SOURCE_DIR}/*.c"
    "${ZLIB_SOURCE_DIR}/*.c"
)

file(GLOB SDL_SOURCE_FILES
  "${COMPABILITY_SDL_SOURCE_DIR}/*.c"
  "${CLIENT_SDL_SOURCE_DIR}/*.c"
)

file(GLOB NML_SOURCE_FILES
  "${COMPABILITY_NML_SOURCE_DIR}/*.c"
  "${COMPABILITY_NML_SOURCE_DIR}/*.cpp"
  "${CLIENT_NML_SOURCE_DIR}/*.c"
  "${CLIENT_NML_SOURCE_DIR}/*.cpp"
)


file(GLOB GAME_SOURCE_FILES
    "${GAME_SOURCE_DIR}/*.c"
    "${COMMON_SHARED_SOURCE_DIR}/*.c"
    "${PLAYER_SOURCE_DIR}/*.c"
    "${SAVE_SOURCE_DIR}/*.c"
)

file(GLOB_RECURSE MONSTER_SOURCE_FILES
    "${MONSTER_SOURCE_DIR}/*.c"
)

add_library(${QUAKE2_CORE} STATIC ${COMMON_SOURCE_FILES})

#list(APPEND ${UNIX_SOURCE_DIR} asan)

add_executable(${PROJECT_NAME} ${SDL_SOURCE_FILES})

add_executable(${NET_QUAKE2} ${NML_SOURCE_FILES})

add_library(${GAME_API} SHARED  ${GAME_SOURCE_FILES} ${MONSTER_SOURCE_FILES})
SET_TARGET_PROPERTIES(${GAME_API} PROPERTIES PREFIX "")

#target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
target_include_directories(${QUAKE2_CORE}
  PUBLIC
    ${INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${OPENGLES2_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/Engine/External/include 
    ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources 
    ${PROJECT_SOURCE_DIR}/Engine/Sources/Compatibility 
    ${PROJECT_SOURCE_DIR}/Engine/Sources/Compatibility/OpenGLES/Includes
)

set_target_properties( ${QUAKE2_CORE}
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties( ${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set_target_properties( ${NET_QUAKE2}
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

list(APPEND BUILD_OPTIONS -DARCH=\"i386\" -DOSTYPE=\"Linux\" -DNOUNCRYPT -DZIP -D_GNU_SOURCE=1
-DEGLW_GLES2 -MP -ffast-math -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-unused-but-set-variable 
-Wno-switch -Wno-missing-field-initializers -Wno-sign-compare 
-Wno-logical-not-parentheses
-Wno-misleading-indentation -Wno-shift-negative-value -Wno-implicit-fallthrough
$<$<COMPILE_LANGUAGE:C>:-Wno-implicit-function-declaration> $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>
$<$<COMPILE_LANGUAGE:C>:-Wno-absolute-value> $<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion> 
$<$<COMPILE_LANGUAGE:C>:-Wno-duplicate-decl-specifier>
-Wno-address -fPIC -fvisibility=hidden)

set(CLANG_15 OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      list(APPEND BUILD_OPTIONS  -Wno-pointer-bool-conversion  -Wno-tautological-constant-out-of-range-compare)
      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0")
        message("Detect CLANG 15")
        set(CLANG_15 ON)
        list(APPEND BUILD_OPTIONS -Wno-deprecated-non-prototype -Wno-array-parameter)
        #target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-non-prototype -Wno-array-parameter)
      endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
list(APPEND BUILD_OPTIONS -Wno-format-overflow -Wno-bool-compare -Wno-builtin-declaration-mismatch -Wno-format-truncation  -Wno-error=restrict -Wno-restrict  
  )
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0")
    message("Detect GCC 12")
    list(APPEND BUILD_OPTIONS -Wno-error=array-parameter)
  #target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-non-prototype -Wno-array-parameter)
  endif()
#target_compile_options(${PROJECT_NAME} PRIVATE -Wno-format-overflow -Wno-bool-compare -Wno-builtin-declaration-mismatch -Wno-format-truncation)
endif()

target_compile_options(${QUAKE2_CORE} PRIVATE ${BUILD_OPTIONS})
target_compile_options(${PROJECT_NAME} PRIVATE ${BUILD_OPTIONS})
target_compile_options(${NET_QUAKE2} PRIVATE ${BUILD_OPTIONS})


  target_include_directories(${GAME_API}
    PUBLIC
      ${PROJECT_SOURCE_DIR}/Engine/External/include 
      ${PROJECT_SOURCE_DIR}/Ports/Quake2/Sources 
  )

  target_compile_options(${GAME_API} PRIVATE -DARCH=\"i386\" -DOSTYPE=\"Linux\" -ffast-math -Wall -Wextra -g -fPIC -std=c99 -Wno-unused-function 
    -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-switch -Wno-missing-field-initializers -fPIC 
    -fvisibility=hidden)

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
if(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$"))
#message(${CMAKE_CXX_COMPILER_ID})
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=leak,address -D__DEBUG)
target_compile_options(${NET_QUAKE2} PRIVATE -fsanitize=leak,address -D__DEBUG)
target_compile_options(${GAME_API} PRIVATE -fsanitize=leak,address -D__DEBUG)
list(APPEND library asan)
list(APPEND game_library asan)
list(APPEND net_library asan)
endif()
endif()
#add_compile_options("-O3")
endif()

list(APPEND library ${SDL2_LIBRARIES} ${OPENGLES2_LIBRARY} m EGL)
list(APPEND net_library ${SDL2_LIBRARIES} m netGL netML EGL)

target_link_libraries( ${PROJECT_NAME}
  PRIVATE
  ${QUAKE2_CORE}
  ${library}
)

target_link_libraries( ${NET_QUAKE2}
  PRIVATE
  ${QUAKE2_CORE}
  ${net_library}
)

target_link_libraries( ${GAME_API}
  PRIVATE
  ${game_library}
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/baseq2)

add_custom_command(TARGET ${GAME_API} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${GAME_API}> ${CMAKE_CURRENT_BINARY_DIR}/baseq2/)
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/bin/)

#install(TARGETS ${PROJECT_NAME}  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)