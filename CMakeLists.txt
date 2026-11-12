cmake_minimum_required(VERSION 3.16)  # CMake version in Ubuntu 22.04 LTS is 3.22
project(oak_ffc_sync VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-deprecated-declarations -Wno-maybe-uninitialized)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(camera_info_manager REQUIRED)
find_package(image_transport REQUIRED)
find_package(depthai_ros_msgs REQUIRED)
find_package(depthai_bridge REQUIRED)
find_package(depthai CONFIG REQUIRED)
find_package(OpenCV 4 REQUIRED COMPONENTS imgproc highgui)

add_executable(oak_ffc_sync src/oak_ffc_sync_publisher.cpp)
target_include_directories(oak_ffc_sync PRIVATE ${OpenCV_INCLUDE_DIRS})
ament_target_dependencies(oak_ffc_sync
  rclcpp
  sensor_msgs
  std_msgs
  cv_bridge
  camera_info_manager
  image_transport
  depthai_ros_msgs
  depthai_bridge
)
target_link_libraries(oak_ffc_sync
  depthai::core
  opencv_imgproc
  opencv_highgui
)

install(TARGETS oak_ffc_sync
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch params rviz
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
