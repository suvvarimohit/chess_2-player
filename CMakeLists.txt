cmake_minimum_required(VERSION 3.20)
project(chess LANGUAGES CXX)

if(EMSCRIPTEN)
  add_executable(chess_web src/main.cpp)
  target_compile_features(chess_web PRIVATE cxx_std_20)
  target_compile_definitions(chess_web PRIVATE CHESS_WEB_BUILD=1)
  set_target_properties(
    chess_web
    PROPERTIES
      CXX_EXTENSIONS OFF
      OUTPUT_NAME chess_core
      SUFFIX ".js"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web"
  )
  target_link_options(
    chess_web
    PRIVATE
      "-sMODULARIZE=1"
      "-sEXPORT_ES6=1"
      "-sEXPORT_NAME=createChessModule"
      "-sALLOW_MEMORY_GROWTH=1"
      "-sNO_EXIT_RUNTIME=1"
      "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString']"
  )

  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/web/assets/fonts")
  configure_file(web/index.html "${CMAKE_BINARY_DIR}/web/index.html" COPYONLY)
  configure_file(web/app.js "${CMAKE_BINARY_DIR}/web/app.js" COPYONLY)
  configure_file(web/style.css "${CMAKE_BINARY_DIR}/web/style.css" COPYONLY)
  file(COPY assets/pieces DESTINATION "${CMAKE_BINARY_DIR}/web/assets")
  configure_file(
    assets/fonts/AdwaitaSans-Regular.ttf
    "${CMAKE_BINARY_DIR}/web/assets/fonts/AdwaitaSans-Regular.ttf"
    COPYONLY
  )
else()
  find_package(SFML 3.1 REQUIRED COMPONENTS Graphics Window System)

  add_executable(chess src/main.cpp)
  target_compile_features(chess PRIVATE cxx_std_20)
  target_link_libraries(chess PRIVATE SFML::Graphics SFML::Window SFML::System)
  target_compile_definitions(
    chess PRIVATE CHESS_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
  )
  set_target_properties(chess PROPERTIES CXX_EXTENSIONS OFF)
endif()

if(EMSCRIPTEN)
  set(CHESS_BUILD_TARGET chess_web)
else()
  set(CHESS_BUILD_TARGET chess)
endif()

if(MSVC)
  target_compile_options(${CHESS_BUILD_TARGET} PRIVATE /W4 /permissive-)
else()
  target_compile_options(${CHESS_BUILD_TARGET} PRIVATE -Wall -Wextra -Wpedantic)
endif()
