cmake_minimum_required(VERSION 3.16)
project(crossbring_rt_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SQLITE "Enable SQLite sink if available" ON)
option(ENABLE_ZEROMQ "Enable ZeroMQ publisher if available" OFF)
option(ENABLE_HTTP_SERVER "Enable built-in HTTP server for metrics/UI" ON)
option(ENABLE_CPR "Enable CPR HTTP client for HTTPS AF source" OFF)

include(FetchContent)

# Prefer system packages in containers/CI to reduce network fetch
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
  )
  FetchContent_MakeAvailable(spdlog)
endif()

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

# Header-only HTTP server/client (cpp-httplib) for metrics/dashboard
if(ENABLE_HTTP_SERVER)
  FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
  )
  FetchContent_MakeAvailable(httplib)
endif()

# Optional CPR for HTTPS AF source
if(ENABLE_CPR)
  FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
  )
  set(CPR_BUILD_TESTS OFF CACHE INTERNAL "")
  set(CPR_ENABLE_SSL ON CACHE INTERNAL "")
  FetchContent_MakeAvailable(cpr)
endif()

add_library(crossbring_engine
  src/core/engine.cpp
  src/core/queue.cpp
  src/sources/sensor_simulator.cpp
  src/sources/file_json_source.cpp
  src/sinks/console_sink.cpp
  src/sinks/recent_buffer_sink.cpp
)

target_include_directories(crossbring_engine PUBLIC include)
target_link_libraries(crossbring_engine PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog)

if(SQLite3_FOUND)
  target_sources(crossbring_engine PRIVATE src/sinks/sqlite_sink.cpp)
  target_compile_definitions(crossbring_engine PRIVATE USE_SQLITE)
  target_link_libraries(crossbring_engine PUBLIC SQLite::SQLite3)
endif()

if(ZeroMQ_FOUND)
  target_sources(crossbring_engine PRIVATE src/sinks/zmq_sink.cpp)
  target_include_directories(crossbring_engine PRIVATE ${ZeroMQ_INCLUDE_DIRS})
  target_link_libraries(crossbring_engine PUBLIC ${ZeroMQ_LIBRARIES})
  target_compile_definitions(crossbring_engine PRIVATE USE_ZEROMQ)
endif()

if(ENABLE_HTTP_SERVER)
  target_sources(crossbring_engine PRIVATE src/http/http_server.cpp)
  target_include_directories(crossbring_engine PRIVATE ${httplib_SOURCE_DIR})
  target_compile_definitions(crossbring_engine PRIVATE USE_HTTP_SERVER)
endif()

if(ENABLE_CPR)
  target_sources(crossbring_engine PRIVATE src/sources/af_https_source.cpp)
  target_link_libraries(crossbring_engine PRIVATE cpr::cpr)
  target_compile_definitions(crossbring_engine PRIVATE USE_CPR)
endif()

add_executable(rt_engine apps/rt_engine_main.cpp)
target_link_libraries(rt_engine PRIVATE crossbring_engine)

install(TARGETS rt_engine RUNTIME DESTINATION bin)
install(DIRECTORY configs/ DESTINATION share/crossbring-rt-engine/configs)

enable_testing()

if(ZeroMQ_FOUND)
  add_executable(zmq_sub_demo apps/zmq_sub_demo.cpp)
  target_link_libraries(zmq_sub_demo PRIVATE ${ZeroMQ_LIBRARIES})
  target_include_directories(zmq_sub_demo PRIVATE ${ZeroMQ_INCLUDE_DIRS})
  target_compile_definitions(zmq_sub_demo PRIVATE USE_ZEROMQ)
endif()
