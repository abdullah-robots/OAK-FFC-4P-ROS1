#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>

#include "camera_info_manager/camera_info_manager.hpp"
#include "depthai/depthai.hpp"
#include "image_transport/image_transport.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/image.hpp"

namespace dai {

namespace ros {

namespace ImageMsgs = sensor_msgs::msg;

/**
 * Publishes every frame of a dai::MessageGroup (output of a dai::node::Sync) on its own topic.
 * rosTopics maps the name of each Sync input (e.g. "CAM_A") to its ROS topic (e.g. "~/CAM_A/image").
 * For image messages, each camera also gets a camera_info topic ("~/<name>/camera_info") and a
 * CameraInfoManager in its own sub-namespace ("~/<name>/set_camera_info").
 */
template <class RosMsg>
class SyncBridgePublisher {
   public:
    using ConvertFunc = std::function<void(std::shared_ptr<dai::ImgFrame>, std::deque<RosMsg>&)>;

    using CustomPublisher = typename std::conditional<std::is_same<RosMsg, ImageMsgs::Image>::value,
                                                      std::shared_ptr<image_transport::Publisher>,
                                                      typename rclcpp::Publisher<RosMsg>::SharedPtr>::type;

    SyncBridgePublisher(std::shared_ptr<dai::DataOutputQueue> daiMessageQueue,
                        std::shared_ptr<rclcpp::Node> node,
                        std::map<std::string, std::string> rosTopics,
                        ConvertFunc converter,
                        int queueSize,
                        std::string cameraParamUri = "",
                        bool lazyPublisher = true);

    SyncBridgePublisher(std::shared_ptr<dai::DataOutputQueue> daiMessageQueue,
                        std::shared_ptr<rclcpp::Node> node,
                        std::map<std::string, std::string> rosTopics,
                        ConvertFunc converter,
                        int queueSize,
                        ImageMsgs::CameraInfo cameraInfoData,
                        bool lazyPublisher = true);

    SyncBridgePublisher(const SyncBridgePublisher& other) = delete;
    SyncBridgePublisher& operator=(const SyncBridgePublisher& other) = delete;

    /**
     * Tag dispatcher: image messages are published with image_transport.
     */
    void advertise(int queueSize, std::true_type);

    /**
     * Tag dispatcher: other messages are published with a plain rclcpp publisher.
     */
    void advertise(int queueSize, std::false_type);

    void addPublisherCallback();

    void publishHelper(std::shared_ptr<dai::ImgFrame> inData, const std::string& cameraName);

    void startPublisherThread();

    ~SyncBridgePublisher();

   private:
    /**
     * Adding this callback still lets you consume the data for other processing with the get() function.
     */
    void daiCallback(std::string name, std::shared_ptr<ADatatype> data);

    void publishGroup(const std::shared_ptr<dai::MessageGroup>& group);

    static const std::string LOG_TAG;
    std::shared_ptr<dai::DataOutputQueue> _daiMessageQueue;
    ConvertFunc _converter;

    std::shared_ptr<rclcpp::Node> _node;
    image_transport::ImageTransport _it;
    ImageMsgs::CameraInfo _cameraInfoData;
    std::map<std::string, CustomPublisher> _rosPublisher;
    std::map<std::string, rclcpp::Publisher<ImageMsgs::CameraInfo>::SharedPtr> _cameraInfoPublisher;
    std::map<std::string, std::shared_ptr<rclcpp::Node>> _cameraInfoNodes;
    std::map<std::string, std::unique_ptr<camera_info_manager::CameraInfoManager>> _camInfoManager;

    std::thread _readingThread;
    std::map<std::string, std::string> _rosTopics;
    std::string _cameraParamUri;
    bool _isCallbackAdded = false;
    bool _isImageMessage = false;  // used to enable the camera info manager
    bool _lazyPublisher = true;
};

template <class RosMsg>
const std::string SyncBridgePublisher<RosMsg>::LOG_TAG = "SyncBridgePublisher";

template <class RosMsg>
SyncBridgePublisher<RosMsg>::SyncBridgePublisher(std::shared_ptr<dai::DataOutputQueue> daiMessageQueue,
                                                 std::shared_ptr<rclcpp::Node> node,
                                                 std::map<std::string, std::string> rosTopics,
                                                 ConvertFunc converter,
                                                 int queueSize,
                                                 std::string cameraParamUri,
                                                 bool lazyPublisher)
    : _daiMessageQueue(daiMessageQueue),
      _converter(converter),
      _node(node),
      _it(node),
      _rosTopics(rosTopics),
      _cameraParamUri(cameraParamUri),
      _lazyPublisher(lazyPublisher) {
    advertise(queueSize, std::is_same<RosMsg, ImageMsgs::Image>{});
}

template <class RosMsg>
SyncBridgePublisher<RosMsg>::SyncBridgePublisher(std::shared_ptr<dai::DataOutputQueue> daiMessageQueue,
                                                 std::shared_ptr<rclcpp::Node> node,
                                                 std::map<std::string, std::string> rosTopics,
                                                 ConvertFunc converter,
                                                 int queueSize,
                                                 ImageMsgs::CameraInfo cameraInfoData,
                                                 bool lazyPublisher)
    : _daiMessageQueue(daiMessageQueue),
      _converter(converter),
      _node(node),
      _it(node),
      _cameraInfoData(cameraInfoData),
      _rosTopics(rosTopics),
      _lazyPublisher(lazyPublisher) {
    advertise(queueSize, std::is_same<RosMsg, ImageMsgs::Image>{});
}

template <class RosMsg>
void SyncBridgePublisher<RosMsg>::advertise(int queueSize, std::false_type) {
    for(auto& rosTopic : _rosTopics) {
        _rosPublisher[rosTopic.first] = _node->create_publisher<RosMsg>(rosTopic.second, queueSize);
    }
}

template <class RosMsg>
void SyncBridgePublisher<RosMsg>::advertise(int queueSize, std::true_type) {
    for(auto& rosTopic : _rosTopics) {
        const auto& camName = rosTopic.first;
        const auto& topic = rosTopic.second;
        if(!camName.empty()) {
            _isImageMessage = true;
            // One CameraInfoManager per camera, each in its own sub-namespace so their set_camera_info services don't clash
            _cameraInfoNodes[camName] = _node->create_sub_node(camName);
            _camInfoManager[camName] =
                std::make_unique<camera_info_manager::CameraInfoManager>(_cameraInfoNodes[camName].get(), camName, _cameraParamUri);
            if(_cameraParamUri.empty()) {
                _camInfoManager[camName]->setCameraInfo(_cameraInfoData);
            }
            _cameraInfoPublisher[camName] = _node->create_publisher<ImageMsgs::CameraInfo>("~/" + camName + "/camera_info", queueSize);
        }
        _rosPublisher[camName] = std::make_shared<image_transport::Publisher>(_it.advertise(topic, queueSize));
    }
}

template <class RosMsg>
void SyncBridgePublisher<RosMsg>::publishGroup(const std::shared_ptr<dai::MessageGroup>& group) {
    for(auto& rosTopic : _rosTopics) {
        auto frame = group->get<dai::ImgFrame>(rosTopic.first);
        if(frame) publishHelper(frame, rosTopic.first);
    }
}

template <class RosMsg>
void SyncBridgePublisher<RosMsg>::daiCallback(std::string /*name*/, std::shared_ptr<ADatatype> data) {
    auto group = std::dynamic_pointer_cast<dai::MessageGroup>(data);
    if(group) publishGroup(group);
}

template <class RosMsg>
void SyncBridgePublisher<RosMsg>::startPublisherThread() {
    if(_isCallbackAdded) {
        throw std::runtime_error(
            "addPublisherCallback() adds a callback to the depthai queue which handles the publishing, "
            "so there is no need to start the thread with startPublisherThread()");
    }

    _readingThread = std::thread([this]() {
        while(rclcpp::ok()) {
            bool timedOut = false;
            auto group = _daiMessageQueue->get<dai::MessageGroup>(std::chrono::milliseconds(100), timedOut);
            if(timedOut || group == nullptr) continue;
            publishGroup(group);
        }
    });
}

template <class RosMsg>
void SyncBridgePublisher<RosMsg>::addPublisherCallback() {
    _daiMessageQueue->addCallback(std::bind(&SyncBridgePublisher<RosMsg>::daiCallback, this, std::placeholders::_1, std::placeholders::_2));
    _isCallbackAdded = true;
}

template <class RosMsg>
void SyncBridgePublisher<RosMsg>::publishHelper(std::shared_ptr<dai::ImgFrame> inDataPtr, const std::string& cameraName) {
    std::deque<RosMsg> opMsgs;

    size_t infoSubCount = 0, mainSubCount = 0;
    if(_isImageMessage) {
        infoSubCount = _cameraInfoPublisher.at(cameraName)->get_subscription_count();
    }
    mainSubCount = _rosPublisher.at(cameraName)->getNumSubscribers();

    if(!_lazyPublisher || (mainSubCount > 0 || infoSubCount > 0)) {
        _converter(inDataPtr, opMsgs);

        while(opMsgs.size()) {
            RosMsg currMsg = opMsgs.front();
            if(mainSubCount > 0) {
                _rosPublisher.at(cameraName)->publish(currMsg);
            }

            if(infoSubCount > 0) {
                auto localCameraInfo = _camInfoManager.at(cameraName)->getCameraInfo();
                localCameraInfo.header.stamp = currMsg.header.stamp;
                localCameraInfo.header.frame_id = currMsg.header.frame_id;
                _cameraInfoPublisher.at(cameraName)->publish(localCameraInfo);
            }
            opMsgs.pop_front();
        }
    }
}

template <class RosMsg>
SyncBridgePublisher<RosMsg>::~SyncBridgePublisher() {
    if(_readingThread.joinable()) _readingThread.join();
}

}  // namespace ros

namespace rosBridge = ros;

}  // namespace dai
