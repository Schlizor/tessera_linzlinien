#pragma once

#include <string>

#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::linz_departures {

class LinzDepartures : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_details(const std::string &title, const std::string &text) {
    detail_title_ = title;
    detail_text_ = text;
  }
  const std::string &detail_title() const { return detail_title_; }
  const std::string &detail_text() const { return detail_text_; }

 private:
  std::string detail_title_;
  std::string detail_text_;
};

}  // namespace esphome::linz_departures
