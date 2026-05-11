cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(web_agent VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WEB_AGENT_BUILD_TESTS "Build unit tests" ON)

find_package(CURL QUIET)
if(CURL_FOUND)
  message(STATUS "libcurl found: HTTPS client enabled")
else()
  message(STATUS "libcurl not found: build will use mock network client")
endif()

add_library(web_agent_lib
  src/core/Agent.cpp
  src/config/ConfigManager.cpp
  src/network/NetworkClient.cpp
  src/task/TaskManager.cpp
  src/executor/Executor.cpp
  src/file/FileManager.cpp
  src/logger/Logger.cpp
)

target_include_directories(web_agent_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CURL_FOUND)
  target_compile_definitions(web_agent_lib PUBLIC WEB_AGENT_HAS_CURL=1)
  target_link_libraries(web_agent_lib PUBLIC CURL::libcurl)
endif()

add_executable(web_agent src/main.cpp)

set_target_properties(web_agent PROPERTIES OUTPUT_NAME "web-agent")
target_link_libraries(web_agent PRIVATE web_agent_lib)

install(TARGETS web_agent
  RUNTIME DESTINATION bin
)
install(DIRECTORY config DESTINATION .)
install(FILES README.md DESTINATION .)

if(WEB_AGENT_BUILD_TESTS)
  enable_testing()

  add_executable(test_config tests/test_config.cpp)
  target_include_directories(test_config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_config PRIVATE web_agent_lib)
  add_test(NAME test_config COMMAND test_config)

  add_executable(test_network tests/test_network.cpp)
  target_include_directories(test_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_network PRIVATE web_agent_lib)
  add_test(NAME test_network COMMAND test_network)

  add_executable(test_executor tests/test_executor.cpp)
  target_include_directories(test_executor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_executor PRIVATE web_agent_lib)
  add_test(NAME test_executor COMMAND test_executor)

  add_executable(test_task tests/test_task.cpp)
  target_include_directories(test_task PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_task PRIVATE web_agent_lib)
  add_test(NAME test_task COMMAND test_task)
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "web-agent")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
  set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
  set(CPACK_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
