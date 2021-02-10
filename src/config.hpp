#pragma once
#include <string>

#include "CppClientNode.hpp"
#include "WebVizConstants.hpp"
#include "topic_config.hpp"
#include "structs.hpp"

using namespace topic_config;

namespace config {
static const std::string ros_node_name = "robofleet_client";

// URL of robofleet_server instance (ignored in direct mode)
static const std::string host_url = "ws://localhost:8080";
// AMRL Robofleet server URL
// static const std::string host_url = "ws://10.0.0.1:8080";

/**
 * Anti-backpressure for normal mode.
 * Uses Websocket PING/PONG protocol to gauge when server has actually received
 * a message. If true, wait for PONGs before sending more messages.
 */
static const bool wait_for_pongs = true;

/**
 * If wait_for_acks, how many more messages to send before waiting for first
 * PONG? This can be set to a value greater than 0 to compensate for network
 * latency and fully saturate available bandwidth, but if it is set too high, it
 * could cause message lag.
 */
static const uint64_t max_queue_before_waiting = 0;

/**
 * Whether to run a Websocket server instead of a client, to bypass the need
 * for a centralized instance of robofleet_server.
 */
static const bool direct_mode = false;
static const quint16 direct_mode_port =
    8080;  // what port to serve on in direct mode
static const quint64 direct_mode_bytes_per_sec =
    2048000;  // avoid network backpressure in direct mode: sets maximum upload
              // speed

/**
 * @brief Configure which topics and types of messages the client will handle.
 *
 * You should use the .configure() method on the RosClientNode, and supply
 * either a SendLocalTopic or a ReceiveRemoteTopic config.
 *
 * To properly integrate with Robofleet, you need to run this client with a
 * ROS namespace representing the robot's name.
 *
 * Absolute topic names begin with a "/"; they will not be prefixed with the
 * current ROS namespace (robot name). Many of your local ROS nodes may publish
 * on absolute-named topics.
 * Most topics must be relative (not begin with "/") on the server side to
 * avoid name collisions between robots.
 *
 * Tips:
 * - When SENDING TO or RECEIVING FROM the server, the topic name should almost
 *   always be relative to avoid name collisions between robots.
 * - When SENDING FROM or RECEIVING TO a local topic, the topic name will often
 *   need to be absolute since many ROS nodes might not use namespaces.
 * - To send to a special webviz topic, make use of webviz_constants.
 */
//static void configure_msg_types(RosClientNode& cn) { }
static void configure_msg_types(CppClientNode& cn) {
  // Read all of the above documentation before modifying

  // Add additional topics to subscribe and publish here.
  // remote robot info
  cn.configure(ReceiveRemoteTopic<AMRLStatus>()
                   .from("/maxbot0/status")
                   .to("/local/status"));
  cn.configure(ReceiveRemoteTopic<AMRLStatus>()
                   .from("/maxbot1/status")
                   .to("/local/status"));
  
}
}  // namespace config
