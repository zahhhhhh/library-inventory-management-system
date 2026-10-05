cmake_minimum_required(VERSION 3.16)

project(LIMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lims-client
    src/client/main.cpp
    src/client/CliApp.cpp
    src/client/ClientSession.cpp
    src/net/Socket.cpp
    src/protocol/Protocol.cpp
)

add_executable(lims-server
    src/server/main.cpp
    src/server/LimsServer.cpp
    src/server/ClientHandler.cpp
    src/inventory/InventoryService.cpp
    src/persistence/LimsStore.cpp
    src/net/Socket.cpp
    src/protocol/Protocol.cpp
)

target_include_directories(lims-client PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(lims-server PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(WIN32)
    target_link_libraries(lims-client PRIVATE ws2_32)
    target_link_libraries(lims-server PRIVATE ws2_32)
endif()
