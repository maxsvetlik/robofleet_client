#pragma once

#include <flatbuffers/flatbuffers.h>
#include <schema_generated.h>

#include <QObject>
#include <QDebug>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <iostream>

#include "MessageScheduler.hpp"
#include "decode.hpp"
#include "encode.hpp"
#include "topic_config.hpp"


class CppClientNode : public QObject {
  Q_OBJECT
  
  struct TopicParams {
    double priority;
    bool no_drop;
  };

  using TopicString = std::string;
  using MsgTypeString = std::string;
  std::vector<TopicString> pub_remote_topics;
  // Map from topic name to pair of <last publication time, publication
  // interval?(sec)>
  std::unordered_map<
      TopicString,
      std::pair<
          std::chrono::time_point<std::chrono::high_resolution_clock>, double>>
      rate_limits;
  std::unordered_map<TopicString, TopicParams> topic_params;
  std::unordered_map<
      MsgTypeString, std::function<void(const QByteArray&, const TopicString&)>>
      pub_fns;

  /**
   * @brief Emit a ros_message_encoded() signal given a message and metadata.
   *
   * @tparam T type of msg
   * @param msg message to encode
   * @param msg_type type of the message; can be obtained using
   * ros::message_traits::DataType
   * @param from_topic topic on which message was received
   * @param to_topic remote topic to send to
   */
  template <typename T>
  void encode_ros_msg(
      const T& msg, const std::string& msg_type, const std::string& from_topic,
      const std::string& to_topic) {
    // check rate limits
    
    if (rate_limits.count(from_topic)) {
      auto& rate_info = rate_limits[from_topic];
      const auto curr_time = std::chrono::high_resolution_clock::now();
      const auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              curr_time - rate_info.first);
      const double elapsed_sec = duration.count() / 1000.0;

      if (elapsed_sec > rate_info.second) {
        rate_info.first = curr_time;
      } else {
        return;
      }
    }

    // encode message
    flatbuffers::FlatBufferBuilder fbb;
    //std::cout << msg << std::endl;
    //ROS_INFO_STREAM(msg);
    auto metadata = encode_metadata(fbb, msg_type, to_topic);
    auto root_offset = encode<T>(fbb, msg, metadata);
    fbb.Finish(flatbuffers::Offset<void>(root_offset));
    const QByteArray data{
        reinterpret_cast<const char*>(fbb.GetBufferPointer()),
        static_cast<int>(fbb.GetSize())};
    const TopicParams& params = topic_params[from_topic];
    Q_EMIT ros_message_encoded(
        QString::fromStdString(to_topic),
        data,
        params.priority,
        params.no_drop);
    
  }

  void set_rate_limit(const std::string& from_topic, double rate_limit_hz) {
    /*
    const std::string full_from_topic = ros::names::resolve(from_topic);
    double publish_interval_sec = 1.0 / rate_limit_hz;
    rate_limits[full_from_topic] = std::make_pair(
        std::chrono::high_resolution_clock::now(), publish_interval_sec);
    */
  }

  /**
   * @brief Set up pub/sub for a particular message type and topic.
   *
   * Creates a subscriber to the given topic, and emits ros_message_encoded()
   * signals for each message received.
   * @tparam T the ROS message type
   * @param from_topic the topic to subscribe to
   * @param to_topic the topic to publish to the server
   * @param max_publish_rate_hz the maximum number of messages per second to
   * publish on this topic
   */
  template <typename T>
  void register_local_msg_type(
      const std::string& from_topic, const std::string& to_topic) {
  }

  /**
   * @brief Set up remote publishing for a particular message type and topic
   * sent from the server.
   *
   * Sets up the client to publish to `to_topic` whenever it
   * recieves a message of type `from_topic` from the remote server.
   * @tparam T the ROS message type
   * @param from_topic the remote topic to expect
   * @param to_topic the local topic to publish to
   */
  // NOTE this isn't really a template anymore, as its specific for 'subscription'
  // registration, since that's the only message needed
  template <typename T>
  void register_remote_msg_type(
      const std::string& from_topic, const std::string& to_topic) {
    // apply remapping to encode full topic name
    const std::string full_from_topic = from_topic;
    const std::string full_to_topic = to_topic;
    const std::string& msg_type = "amrl_msgs/RobofleetStatus";
    //std::cout << msg_type.c_str() << std::endl;
    // create function that will decode and publish a T message to any topic
    if (pub_fns.count(msg_type) == 0) {
      pub_fns[msg_type] = [this, full_to_topic](
                              const QByteArray& data,
                              const std::string& msg_type) {
        const auto* root =
            flatbuffers::GetRoot<fb::amrl_msgs::RobofleetStatus>(
                data.data());

	AMRLStatus amrls;
	amrls.battery_level = root->battery_level();
	amrls.is_ok = root->is_ok();
	amrls.location = root->location()->str();
	amrls.status = root->status()->str();
	std::cout << amrls.battery_level << std::endl;
	std::cout << amrls.is_ok << std::endl;
	std::cout << amrls.location <<  std::endl;
	std::cout << amrls.status <<  std::endl;
      };
    }

    pub_remote_topics.push_back(full_from_topic);
  }

 Q_SIGNALS:
  void ros_message_encoded(
      const QString& topic, const QByteArray& data, double priority,
      bool no_drop);

 public Q_SLOTS:
  /**
   * @brief Attempt to decode an publish message data.
   *
   * Must call register_msg_type<T> before a message of type T can be decoded.
   * @param data the Flatbuffer-encoded message data
   */
  void decode_net_message(const QByteArray& data) {
    // extract metadata
    const fb::MsgWithMetadata* msg =
        flatbuffers::GetRoot<fb::MsgWithMetadata>(data.data());
    const std::string& msg_type = msg->__metadata()->type()->str();
    const std::string& topic = msg->__metadata()->topic()->str();
    std::cout << "received " << msg_type << " message on " << topic << std::endl;
   
    // try to publish
    if (pub_fns.count(msg_type) == 0) { return;}

    // TODO Probably want to add this into the output struct
    std::cout << "For robot " << get_robot_name_from_topic(topic) << std::endl;
    pub_fns[msg_type](data, topic);
  }

 
 public:
   /*
    * Returns the robot name from the fully qualified topic name that is used 
    * to get the message from robofleet. 
    * Assumes the first token block is the robot name.
    */
   std::string get_robot_name_from_topic(std::string topic){
      std::string token = topic.erase(0, topic.find('/') + 1);
      return token.erase(token.find('/'), token.length());  
   }

  /**
   * @brief subscribe to remote messages that were specified in the config by
   * calling register_remote_msg_type. This function should run once a websocket
   * connection has been established
   */
  void subscribe_remote_msgs() {
    
    for (auto topic : pub_remote_topics) {
      printf(
          "Registering for remote subscription to topic %s\n", topic.c_str());
      // Now, subscribe to the appropriate remote message
      AMRLSubscription sub_msg;
      sub_msg.topic_regex = topic;
      sub_msg.action = 1;
      encode_ros_msg<AMRLSubscription>(
          sub_msg,
          "amrl_msgs/RobofleetSubscription",
          "/subscriptions",
          "/subscriptions");
    }
  }

  template <typename T>
  void configure(const topic_config::ReceiveRemoteTopic<T>& config) {
    config.assert_valid();
    register_remote_msg_type<T>(config.from, config.to);
  }

  CppClientNode() {
    std::cerr << "Started CPP Client" << std::endl;
  }

};
