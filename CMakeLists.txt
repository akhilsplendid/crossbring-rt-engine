cmake_minimum_required(VERSION 3.16)
project(crossbring_rt_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SQLITE "Enable SQLite sink if available" ON)
option(ENABLE_ZEROMQ "Enable ZeroMQ publisher if available" OFF)

include(FetchContent)

# Dependencies: nlohmann_json (header-only) and spdlog (header-only)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# Optional: SQLite3
if(ENABLE_SQLITE)
  find_package(SQLite3 QUIET)
endif()

# Optional: ZeroMQ
if(ENABLE_ZEROMQ)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(ZeroMQ libzmq QUIET)
  endif()
endif()

add_library(crossbring_engine
  src/core/engine.cpp
  src/core/queue.cpp
  src/sources/sensor_simulator.cpp
  src/sources/file_json_source.cpp
  src/sinks/console_sink.cpp
)

target_include_directories(crossbring_engine PUBLIC include)
target_link_libraries(crossbring_engine PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog)

if(SQLite3_FOUND)
  target_sources(crossbring_engine PRIVATE src/sinks/sqlite_sink.cpp)
  target_compile_definitions(crossbring_engine PRIVATE USE_SQLITE)
  target_link_libraries(crossbring_engine PRIVATE SQLite::SQLite3)
endif()

if(ZeroMQ_FOUND)
  target_sources(crossbring_engine PRIVATE src/sinks/zmq_sink.cpp)
  target_include_directories(crossbring_engine PRIVATE ${ZeroMQ_INCLUDE_DIRS})
  target_link_libraries(crossbring_engine PRIVATE ${ZeroMQ_LIBRARIES})
  target_compile_definitions(crossbring_engine PRIVATE USE_ZEROMQ)
endif()

add_executable(rt_engine apps/rt_engine_main.cpp)
target_link_libraries(rt_engine PRIVATE crossbring_engine)

install(TARGETS rt_engine RUNTIME DESTINATION bin)
install(DIRECTORY configs/ DESTINATION share/crossbring-rt-engine/configs)

enable_testing()
