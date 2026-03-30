# CMake generated Testfile for 
# Source directory: /Users/daniil/VS progects/Web-agent
# Build directory: /Users/daniil/VS progects/Web-agent/build-merged
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_config "/Users/daniil/VS progects/Web-agent/build-merged/test_config")
set_tests_properties(test_config PROPERTIES  _BACKTRACE_TRIPLES "/Users/daniil/VS progects/Web-agent/CMakeLists.txt;60;add_test;/Users/daniil/VS progects/Web-agent/CMakeLists.txt;0;")
add_test(test_network "/Users/daniil/VS progects/Web-agent/build-merged/test_network")
set_tests_properties(test_network PROPERTIES  _BACKTRACE_TRIPLES "/Users/daniil/VS progects/Web-agent/CMakeLists.txt;65;add_test;/Users/daniil/VS progects/Web-agent/CMakeLists.txt;0;")
add_test(test_executor "/Users/daniil/VS progects/Web-agent/build-merged/test_executor")
set_tests_properties(test_executor PROPERTIES  _BACKTRACE_TRIPLES "/Users/daniil/VS progects/Web-agent/CMakeLists.txt;70;add_test;/Users/daniil/VS progects/Web-agent/CMakeLists.txt;0;")
subdirs("_deps/nlohmann_json-build")
