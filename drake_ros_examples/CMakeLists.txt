cmake_minimum_required(VERSION 3.10)
project(drake_ros_examples)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-error=deprecated-declarations)
endif()

find_package(ament_cmake_ros REQUIRED)
find_package(drake REQUIRED)
find_package(drake_ros REQUIRED)
find_package(gflags REQUIRED COMPONENTS shared)

add_subdirectory(examples/hydroelastic)
add_subdirectory(examples/iiwa_manipulator)
add_subdirectory(examples/multirobot)
add_subdirectory(examples/rs_flip_flop)

if(BUILD_TESTING)
  find_package(ament_cmake_clang_format REQUIRED)
  find_package(ament_cmake_cpplint REQUIRED)
  find_package(ament_cmake_test REQUIRED)
  find_package(Python3 COMPONENTS Interpreter)

  macro(add_python_test test_name test_file)
    ament_add_test("${test_name}"
      COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${test_file}"
        "--junit-xml=${AMENT_TEST_RESULTS_DIR}/drake_ros_examples/${test_name}.xml"
        "--junit-prefix=drake_ros_examples")
  endmacro()

  add_python_test(parse_ros_model_test test/parse_ros_model_test.py)

  ament_clang_format(CONFIG_FILE .clang-format)
  ament_cpplint()
endif()

ament_package()
