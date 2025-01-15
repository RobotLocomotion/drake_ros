# Copyright (c) 2021, Massachusetts Institute of Technology.
# Copyright (c) 2021, Toyota Research Institute.
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)
project(drake_example_pendulum)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake CONFIG REQUIRED)
find_package(drake CONFIG REQUIRED PATHS /opt/drake)

add_executable(${PROJECT_NAME}
  src/drake_example_pendulum.cc
)

target_link_libraries(${PROJECT_NAME} drake::drake)

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

ament_package()
