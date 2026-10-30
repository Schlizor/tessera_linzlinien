#include "linz_departures.h"

#include <algorithm>
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
    // Choice options are sent as {value, label}; the value is the Linz AG stop ID.
    stop_ = c.options["stop"]["value"] | "";
    for (int i = 0; i < kMaxRows; ++i) make_row(rows_[i], c.parent);
  }

  void on_state(JsonObjectConst data) override {
    departures_.clear();
    wait_ = data["wait"] | "";
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      if (departures_.size() >= kMaxRows) break;
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

    const int row_height = std::max(ui::line_height(Font::TITLE), ui::line_height(Font::BODY)) + ui::px(8);
    const int visible = std::min(kMaxRows, std::max(1, height_ / row_height));
    for (int i = 0; i < kMaxRows; ++i) {
      if (i >= visible || i >= static_cast<int>(departures_.size())) {
        hide_row(rows_[i]);
        continue;
      }
      const DepartureRow &departure = departures_[i];
      std::string when;
      if (departure.cancelled) {
        when = plugin_->text("cancelled");
      } else if (epoch && departure.estimated <= epoch + 30) {
        when = plugin_->text("now");
      } else if (epoch && departure.estimated > epoch) {
        when = tessera::format(plugin_->text("minutes"), (departure.estimated - epoch) / 60);
      } else {
        when = "--";
      }
      if (!departure.platform.empty()) when += "  " + departure.platform;
      place_row(rows_[i], departure.line, departure.destination, when, i * row_height);
    }
  }

  void on_theme() override {
    for (auto &row : rows_) {
      lv_obj_set_style_bg_color(row.badge, ui::color(theme::ACCENT), 0);
      ui::set_color(row.line, theme::ON_ACCENT);
      ui::set_color(row.destination, theme::INK);
      ui::set_color(row.when, theme::MUTED);
    }
    on_tick(tessera::epoch());
  }

 private:
  static constexpr int kMaxRows = 8;

  struct Row {
    lv_obj_t *badge{};
    lv_obj_t *line{};
    lv_obj_t *destination{};
    lv_obj_t *when{};
  };

  const tessera::Plugin *plugin_;
  int width_ = 0;
  int height_ = 0;
  std::string stop_;
  std::string wait_;
  std::vector<DepartureRow> departures_;
  Row rows_[kMaxRows];

  void make_row(Row &row, lv_obj_t *parent) {
    row.badge = ui::block(parent, theme::ACCENT);
    row.line = ui::label(row.badge, Font::TITLE, theme::ON_ACCENT);
    row.destination = ui::label(parent, Font::BODY, theme::INK);
    row.when = ui::label(parent, Font::BODY, theme::MUTED);
    lv_label_set_long_mode(row.line, LV_LABEL_LONG_CLIP);
    lv_label_set_long_mode(row.destination, LV_LABEL_LONG_DOT);
    lv_label_set_long_mode(row.when, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(row.when, LV_TEXT_ALIGN_RIGHT, 0);
    hide_row(row);
  }

  void place_row(Row &row, const std::string &line, const std::string &destination, const std::string &when, int y) {
    const int row_height = std::max(ui::line_height(Font::TITLE), ui::line_height(Font::BODY)) + ui::px(8);
    const int pad = ui::px(6);
    const int badge_width = std::min(width_ / 3, std::max(ui::px(28), ui::text_width(line, Font::TITLE) + pad * 2));
    const int when_width = std::min(width_ / 2, ui::text_width(when, Font::BODY) + ui::px(2));
    const int destination_x = badge_width + ui::px(8);
    const int destination_width = std::max(ui::px(20), width_ - destination_x - when_width - ui::px(8));
    const int text_y = y + (row_height - ui::line_height(Font::BODY)) / 2;

    lv_obj_clear_flag(row.badge, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(row.line, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(row.destination, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(row.when, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(row.badge, badge_width, row_height);
    lv_obj_set_pos(row.badge, 0, y);
    lv_obj_center(row.line);
    lv_obj_set_size(row.destination, destination_width, ui::line_height(Font::BODY));
    lv_obj_set_pos(row.destination, destination_x, text_y);
    lv_obj_set_size(row.when, when_width, ui::line_height(Font::BODY));
    lv_obj_set_pos(row.when, width_ - when_width, text_y);
    ui::set_text(row.line, line);
    ui::set_text(row.destination, destination);
    ui::set_text(row.when, when);
  }

  void hide_row(Row &row) {
    lv_obj_add_flag(row.badge, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(row.line, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(row.destination, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(row.when, LV_OBJ_FLAG_HIDDEN);
  }

  void paint_message(const std::string &message) {
    for (int i = 0; i < kMaxRows; ++i) hide_row(rows_[i]);
    lv_obj_clear_flag(rows_[0].destination, LV_OBJ_FLAG_HIDDEN);
    ui::set_text(rows_[0].destination, message);
    lv_obj_set_pos(rows_[0].destination, 0, std::max(0, (height_ - ui::line_height(Font::BODY)) / 2));
    lv_obj_set_width(rows_[0].destination, width_);
  }
};

void LinzDepartures::setup() {
  add_tile("departures", [this]() { return new DeparturesTile(this); });
}

}  // namespace esphome::linz_departures
