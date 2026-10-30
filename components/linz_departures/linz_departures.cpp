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
    for (JsonObjectConst choice : c.options["line"].as<JsonArrayConst>()) {
      lines_.push_back(choice["value"] | "");
    }
    if (lines_.empty()) {
      const std::string single_line = c.options["line"]["value"] | "";
      if (!single_line.empty()) lines_.push_back(single_line);
    }
    direction_a_ = c.options["direction_a"]["value"] | "";
    if (direction_a_.empty()) direction_a_ = c.options["direction"]["value"] | "";
    direction_b_ = c.options["direction_b"]["value"] | "";
    show_platform_ = c.options["show_platform"] | true;
    show_time_ = c.options["show_time"] | true;
    title_ = ui::label(c.parent, Font::TITLE, theme::INK);
    ui::set_text(title_, plugin_->text("departures_title"));
    for (int i = 0; i < kMaxRows; ++i) make_row(rows_[i], c.parent);
  }

  void on_state(JsonObjectConst data) override {
    departures_.clear();
    wait_ = data["wait"] | "";
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      DepartureRow row;
      row.line = item["line"] | "?";
      row.destination = item["destination"] | "?";
      row.estimated = item["estimated"] | (item["planned"] | 0u);
      row.platform = item["platform"] | "";
      row.cancelled = item["cancelled"] | false;
      if (!lines_.empty() && std::find(lines_.begin(), lines_.end(), row.line) == lines_.end()) continue;
      if (!direction_a_.empty() && row.destination != direction_a_ && row.destination != direction_b_) continue;
      if (departures_.size() >= kMaxRows) break;
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

    const int title_height = ui::line_height(Font::TITLE);
    const int row_height = std::max(ui::line_height(Font::TITLE), ui::line_height(Font::BODY)) + ui::px(8);
    const int content_y = title_height + ui::px(8);
    const int visible = std::min(kMaxRows, std::max(1, (height_ - content_y) / row_height));
    lv_obj_clear_flag(title_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(title_, width_, title_height);
    lv_obj_set_pos(title_, 0, 0);
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
      if (show_time_ && departure.estimated) when = tessera::clock_text(departure.estimated) + "  " + when;
      if (show_platform_ && !departure.platform.empty()) when += "  " + departure.platform;
      place_row(rows_[i], departure.line, departure.destination, when, content_y + i * row_height);
    }
  }

  void on_theme() override {
    for (auto &row : rows_) {
      lv_obj_set_style_bg_color(row.badge, ui::color(theme::ACCENT), 0);
      ui::set_color(row.line, theme::ON_ACCENT);
      ui::set_color(row.destination, theme::INK);
      ui::set_color(row.when, theme::MUTED);
    }
    ui::set_color(title_, theme::INK);
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
  std::vector<std::string> lines_;
  std::string direction_a_;
  std::string direction_b_;
  std::string wait_;
  bool show_platform_ = true;
  bool show_time_ = true;
  std::vector<DepartureRow> departures_;
  lv_obj_t *title_{};
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
    lv_obj_clear_flag(title_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(rows_[0].destination, LV_OBJ_FLAG_HIDDEN);
    ui::set_text(rows_[0].destination, message);
    lv_obj_set_pos(rows_[0].destination, 0, std::max(ui::line_height(Font::TITLE) + ui::px(8),
                                                     (height_ - ui::line_height(Font::BODY)) / 2));
    lv_obj_set_width(rows_[0].destination, width_);
  }
};

class NextDepartureTile : public tessera::Tile {
 public:
  explicit NextDepartureTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    stop_ = c.options["stop"]["value"] | "";
    for (JsonObjectConst choice : c.options["line"].as<JsonArrayConst>()) {
      lines_.push_back(choice["value"] | "");
    }
    if (lines_.empty()) {
      const std::string single_line = c.options["line"]["value"] | "";
      if (!single_line.empty()) lines_.push_back(single_line);
    }
    direction_a_ = c.options["direction_a"]["value"] | "";
    direction_b_ = c.options["direction_b"]["value"] | "";
    if (direction_a_.empty()) direction_a_ = c.options["direction"]["value"] | "";
    show_platform_ = c.options["show_platform"] | true;
    show_time_ = c.options["show_time"] | true;
    title_ = ui::label(c.parent, Font::TITLE, theme::INK);
    ui::set_text(title_, plugin_->text("next_departure_title"));
    badge_ = ui::block(c.parent, theme::ACCENT);
    line_ = ui::label(badge_, Font::BODY, theme::ON_ACCENT);
    destination_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    when_ = ui::label(c.parent, Font::VALUE, theme::INK);
    lv_label_set_long_mode(destination_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(when_, LV_TEXT_ALIGN_RIGHT, 0);
  }

  void on_state(JsonObjectConst data) override {
    wait_ = data["wait"] | "";
    departure_ = {};
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      DepartureRow candidate;
      candidate.line = item["line"] | "?";
      candidate.destination = item["destination"] | "?";
      candidate.estimated = item["estimated"] | (item["planned"] | 0u);
      candidate.platform = item["platform"] | "";
      candidate.cancelled = item["cancelled"] | false;
      if (!lines_.empty() && std::find(lines_.begin(), lines_.end(), candidate.line) == lines_.end()) continue;
      if (!direction_a_.empty() && candidate.destination != direction_a_ && candidate.destination != direction_b_) continue;
      departure_ = candidate;
      break;
    }
  }

  void on_tick(uint32_t epoch) override {
    if (stop_.empty()) {
      paint_message(plugin_->text("no_stop"));
      return;
    }
    if (departure_.line.empty()) {
      paint_message(plugin_->text(wait_.empty() ? "no_departures" : "waiting"));
      return;
    }
    const std::string when = departure_.cancelled
                                 ? plugin_->text("cancelled")
                                 : !epoch || departure_.estimated <= epoch + 30
                                       ? plugin_->text("now")
                                       : tessera::format(plugin_->text("minutes"), (departure_.estimated - epoch) / 60);
    ui::set_text(line_, departure_.line);
    std::string destination = departure_.destination;
    if (show_platform_ && !departure_.platform.empty()) destination += "  ·  " + departure_.platform;
    if (show_time_) destination += "  ·  " + when;
    ui::set_text(destination_, destination);
    ui::set_text(when_, show_time_ && departure_.estimated ? tessera::clock_text(departure_.estimated) : when);
    const int title_height = ui::line_height(Font::TITLE);
    const int badge_height = ui::line_height(Font::BODY) + ui::px(8);
    const int badge_width = std::min(width_ / 2, std::max(ui::px(32), ui::text_width(departure_.line, Font::BODY) + ui::px(12)));
    lv_obj_clear_flag(title_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(title_, width_, title_height);
    lv_obj_set_pos(title_, 0, 0);
    lv_obj_set_size(badge_, badge_width, badge_height);
    lv_obj_set_pos(badge_, 0, title_height + ui::px(6));
    lv_obj_center(line_);
    lv_obj_set_size(destination_, width_, ui::line_height(Font::BODY));
    lv_obj_set_pos(destination_, 0, title_height + badge_height + ui::px(12));
    lv_obj_set_size(when_, width_, ui::line_height(Font::VALUE));
    lv_obj_set_pos(when_, 0, std::max(title_height + badge_height + ui::line_height(Font::BODY) + ui::px(16),
                                      height_ - ui::line_height(Font::VALUE)));
    clear_hidden();
  }

  void on_theme() override {
    lv_obj_set_style_bg_color(badge_, ui::color(theme::ACCENT), 0);
    ui::set_color(line_, theme::ON_ACCENT);
    ui::set_color(title_, theme::INK);
    ui::set_color(destination_, theme::MUTED);
    ui::set_color(when_, theme::INK);
    on_tick(tessera::epoch());
  }

 private:
  const tessera::Plugin *plugin_;
  int width_ = 0;
  int height_ = 0;
  std::string stop_;
  std::vector<std::string> lines_;
  std::string direction_a_;
  std::string direction_b_;
  std::string wait_;
  bool show_platform_ = true;
  bool show_time_ = true;
  DepartureRow departure_;
  lv_obj_t *badge_{};
  lv_obj_t *title_{};
  lv_obj_t *line_{};
  lv_obj_t *destination_{};
  lv_obj_t *when_{};

  void clear_hidden() {
    lv_obj_clear_flag(badge_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(line_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(destination_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(when_, LV_OBJ_FLAG_HIDDEN);
  }

  void paint_message(const std::string &message) {
    ui::set_text(destination_, message);
    ui::set_text(line_, "");
    ui::set_text(when_, "");
    lv_obj_add_flag(badge_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(title_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(line_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(destination_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(when_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(destination_, width_, ui::line_height(Font::BODY));
    lv_obj_set_pos(destination_, 0, std::max(0, (height_ - ui::line_height(Font::BODY)) / 2));
  }
};

void LinzDepartures::setup() {
  add_tile("departures", [this]() { return new DeparturesTile(this); });
  add_tile("next", [this]() { return new NextDepartureTile(this); });
}

}  // namespace esphome::linz_departures
