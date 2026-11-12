# Info
ROS 2 Humble driver for the Luxonis OAK-FFC-4P, ported from the ROS1 driver provided by OAK China.

It publishes the four cameras (`CAM_A` to `CAM_D`), hardware-synchronised through the frame-sync input, and the IMU orientation (rotation vector, 400 Hz):

| Topic | Type |
|---|---|
| `/oak_ffc_sync_publisher/CAM_X/image` | `sensor_msgs/msg/Image` |
| `/oak_ffc_sync_publisher/CAM_X/camera_info` | `sensor_msgs/msg/CameraInfo` |
| `/oak_ffc_sync_publisher/imu` | `sensor_msgs/msg/Imu` (orientation only) |

Topics are only published while something subscribes to them.

# Installation
Tested to build on Ubuntu 22.04 with ROS 2 Humble and the depthai v2 packages from the ROS apt repository (`ros-humble-depthai` 2.31, `ros-humble-depthai-bridge` 2.12). The driver uses the depthai v2 API and does not build against depthai v3.

```shell
sudo apt install python3-rosdep python3-colcon-common-extensions
sudo rosdep init
rosdep update
```

```shell
mkdir -p ~/oak_ws/src
# copy this repository into ~/oak_ws/src
cd ~/oak_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release
```

A build check in a Docker container (from the repository root):
```shell
docker build -f docker/Dockerfile -t oak_ffc_sync:humble .
```

# Launch
```shell
source ~/oak_ws/install/setup.bash
ros2 launch oak_ffc_sync oak_ffc_sync.launch.py
```

Launch arguments: `tf_prefix`, `camera_param_uri`, `camera_type` (`color` or `mono`), `resolution`, `auto_exposure_enable`, `exposure_time`, `iso`, and `rviz` (start RViz2, default `true`).

# IMU firmware
`update_imu_frameware.py` prints the IMU firmware versions and then flashes the IMU firmware bundled with the installed `depthai` Python package (depthai v2 API). Flashing can soft-brick the device if interrupted.
