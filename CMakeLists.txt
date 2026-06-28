# Chemiss C++ Backend + HTML Frontend
# A chemistry-based strategy board game

cmake_minimum_required(VERSION 3.16)
project(chemiss-server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/game/element.cpp
    src/game/piece.cpp
    src/game/board.cpp
    src/game/rules.cpp
    src/game/game_state.cpp
    src/network/websocket_server.cpp
    src/utils/json_utils.cpp
)

set(HEADERS
    src/game/element.h
    src/game/piece.h
    src/game/board.h
    src/game/rules.h
    src/game/game_state.h
    src/network/websocket_server.h
    src/utils/json_utils.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Platform-specific socket libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
else()
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

# Copy frontend to build directory
add_custom_target(copy_frontend ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/frontend ${CMAKE_BINARY_DIR}/frontend
)

add_dependencies(${PROJECT_NAME} copy_frontend)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY frontend/ DESTINATION share/chemiss/frontend)
