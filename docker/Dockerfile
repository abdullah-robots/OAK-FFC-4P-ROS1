# Build check for oak_ffc_sync on ROS 2 Humble (Ubuntu 22.04) with depthai v2 from the ROS apt repository.
# Build from the repository root:
#   docker build -f docker/Dockerfile -t oak_ffc_sync:humble .
FROM ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      python3-colcon-common-extensions \
      ros-humble-depthai \
      ros-humble-depthai-bridge \
      ros-humble-camera-info-manager \
      ros-humble-image-transport \
      ros-humble-cv-bridge \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /ws
COPY . /ws/src/oak_ffc_sync
RUN apt-get update && rosdep update --rosdistro humble \
 && rosdep install --from-paths src --ignore-src -r -y --rosdistro humble \
 && rm -rf /var/lib/apt/lists/*
RUN . /opt/ros/humble/setup.sh && colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release
