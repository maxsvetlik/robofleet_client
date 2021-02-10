#pragma once
struct AMRLStatus {
  std::string status;
  bool is_ok;
  float battery_level;
  std::string location;
};

struct AMRLSubscription {
    std::string topic_regex;
    uint8_t action;
};


