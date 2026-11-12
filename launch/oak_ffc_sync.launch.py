"""Launch the OAK-FFC-4P synchronised camera + IMU driver and RViz2 (ROS 2 port of oak_ffc_sync.launch)."""
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, Shutdown
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    package_dir = get_package_share_directory('oak_ffc_sync')

    args = [
        DeclareLaunchArgument('tf_prefix', default_value='oak'),
        DeclareLaunchArgument('camera_type', default_value='color', description="'color' or 'mono'"),
        DeclareLaunchArgument('resolution', default_value='1200p'),
        DeclareLaunchArgument('camera_param_uri', default_value='package://oak_ffc_sync/params/ffc'),
        DeclareLaunchArgument('auto_exposure_enable', default_value='false'),
        DeclareLaunchArgument('exposure_time', default_value='12000', description='Manual exposure time in microseconds'),
        DeclareLaunchArgument('iso', default_value='160', description='Manual ISO sensitivity'),
        DeclareLaunchArgument('rviz', default_value='true', description='Start RViz2'),
    ]

    driver = Node(
        package='oak_ffc_sync',
        executable='oak_ffc_sync',
        name='oak_ffc_sync_publisher',
        output='screen',
        on_exit=Shutdown(),  # same as required="true" in ROS1
        parameters=[{
            'tf_prefix': LaunchConfiguration('tf_prefix'),
            'camera_param_uri': LaunchConfiguration('camera_param_uri'),
            'camera_type': LaunchConfiguration('camera_type'),
            'resolution': LaunchConfiguration('resolution'),
            'auto_exposure_enable': ParameterValue(LaunchConfiguration('auto_exposure_enable'), value_type=bool),
            'exposure_time': ParameterValue(LaunchConfiguration('exposure_time'), value_type=int),
            'iso': ParameterValue(LaunchConfiguration('iso'), value_type=int),
        }],
    )

    rviz = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', os.path.join(package_dir, 'rviz', 'oak_ffc_sync_publisher.rviz')],
        condition=IfCondition(LaunchConfiguration('rviz')),
    )

    return LaunchDescription(args + [driver, rviz])
