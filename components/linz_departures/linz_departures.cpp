#include "linz_departures.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace esphome::linz_departures {

using tessera::Font;
namespace ui = tessera::ui;

struct DepartureRow {
  std::string line;
  std::string destination;
  uint32_t estimated = 0;
  std::string platform;
  bool cancelled = false;
};

class DeparturesTile : public tessera::Tile {
 public:
  explicit DeparturesTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    stop_ = c.options["stop"] | "";
    for (int i = 0; i < 8; ++i) {
      rows_[i].line = ui::label(c.parent, Font::BODY_LARGE, theme::INK);
      rows_[i].destination = ui::label(c.parent, Font::BODY, theme::MUTED);
      lv_obj_set_width(rows_[i].line, width_);
      lv_obj_set_width(rows_[i].destination, width_);
    }
  }

  void on_state(JsonObjectConst data) override {
    departures_.clear();
    wait_ = data["wait"] | "";
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      if (departures_.size() >= 8) break;
      DepartureRow row;
      row.line = item["line"] | "?";
      row.destination = item["destination"] | "?";
      row.estimated = item["estimated"] | (item["planned"] | 0u);
      row.platform = item["platform"] | "";
      row.cancelled = item["cancelled"] | false;
      departures_.push_back(row);
    }
  }

  void on_tick(uint32_t epoch) override {
    if (stop_.empty()) {
      paint_message(plugin_->text("no_stop"));
      return;
    }
    if (departures_.empty()) {
      paint_message(plugin_->text(wait_.empty() ? "no_departures" : "waiting"));
      return;
    }

    const int line_height = ui::line_height(Font::BODY_LARGE);
    const int destination_height = ui::line_height(Font::BODY);
    const int row_height = line_height + destination_height + ui::px(4);
    const int visible = std::min(8, std::max(1, height_ / row_height));
    for (int i = 0; i < 8; ++i) {
      if (i >= visible || i >= static_cast<int>(departures_.size())) {
        lv_obj_add_flag(rows_[i].line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(rows_[i].destination, LV_OBJ_FLAG_HIDDEN);
        continue;
      }
      lv_obj_clear_flag(rows_[i].line, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(rows_[i].destination, LV_OBJ_FLAG_HIDDEN);
      const DepartureRow &row = departures_[i];
      std::string when;
      if (row.cancelled) {
        when = "X";
      } else if (epoch && row.estimated <= epoch + 30) {
        when = plugin_->text("now");
      } else if (epoch && row.estimated > epoch) {
        when = tessera::format(plugin_->text("minutes"), (row.estimated - epoch) / 60);
      } else {
        when = "--";
      }
      std::string line = row.line + "  " + when;
      if (!row.platform.empty()) line += "  " + row.platform;
      ui::set_text(rows_[i].line, line);
      ui::set_text(rows_[i].destination, row.destination);
      lv_obj_set_pos(rows_[i].line, 0, i * row_height);
      lv_obj_set_pos(rows_[i].destination, 0, i * row_height + line_height);
      lv_obj_set_height(rows_[i].line, line_height);
      lv_obj_set_height(rows_[i].destination, destination_height);
    }
  }

  void on_theme() override {
    for (auto &row : rows_) {
      ui::set_color(row.line, theme::INK);
      ui::set_color(row.destination, theme::MUTED);
    }
    on_tick(tessera::epoch());
  }

 private:
  struct Labels { lv_obj_t *line{}; lv_obj_t *destination{}; } rows_[8];
  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0;
  std::string stop_, wait_;
  std::vector<DepartureRow> departures_;

  void paint_message(const std::string &message) {
    for (int i = 0; i < 8; ++i) {
      if (i == 0) {
        lv_obj_clear_flag(rows_[i].line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(rows_[i].destination, LV_OBJ_FLAG_HIDDEN);
        ui::set_text(rows_[i].line, message);
        lv_obj_set_pos(rows_[i].line, 0, std::max(0, (height_ - ui::line_height(Font::BODY_LARGE)) / 2));
      } else {
        lv_obj_add_flag(rows_[i].line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(rows_[i].destination, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
};

void LinzDepartures::setup() {
  add_tile("departures", [this]() { return new DeparturesTile(this); });
}

}  // namespace esphome::linz_departures
