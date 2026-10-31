#include "linz_departures.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace esphome::linz_departures {

using tessera::Font;
namespace ui = tessera::ui;

struct Departure {
  std::string line;
  std::string destination;
  std::string platform;
  std::string info;
  uint32_t planned = 0;
  uint32_t at = 0;
  int delay = 0;
  bool cancelled = false;
};

static void append_info(std::string &result, const std::string &value) {
  if (value.empty() || result.find(value) != std::string::npos) return;
  if (!result.empty()) result += "  ·  ";
  result += value;
}

static void collect_info(JsonVariantConst value, std::string &result) {
  if (value.is<const char *>()) {
    const char *text = value.as<const char *>();
    if (text) append_info(result, text);
    return;
  }
  if (value.is<JsonArrayConst>()) {
    for (JsonVariantConst item : value.as<JsonArrayConst>()) collect_info(item, result);
    return;
  }
  if (!value.is<JsonObjectConst>()) return;
  JsonObjectConst object = value.as<JsonObjectConst>();
  collect_info(object["content"], result);
  collect_info(object["urlText"], result);
  collect_info(object["subtitle"], result);
  collect_info(object["infoLinks"], result);
}

class DeparturesTile : public tessera::Tile {
 public:
  explicit DeparturesTile(LinzDepartures *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    list_ = c.columns >= 2;
    stop_ = c.options["stop"]["value"] | "";
    title_ = c.options["title"]["value"] | "";
    if (title_.empty()) title_ = c.options["title"] | "";
    read_options(c.options);

    head_ = std::max(ui::line_height(Font::TITLE), ui::line_height(Font::BODY)) + ui::px(4);
    gap_ = ui::px(ui::large() ? 8 : 5);
    title_height_ = title_.empty() ? 0 : ui::line_height(Font::TITLE) + ui::px(4);
    if (!title_.empty()) {
      title_label_ = ui::label(c.parent, Font::TITLE, theme::INK);
      ui::set_text(title_label_, title_);
      lv_obj_set_size(title_label_, width_, ui::line_height(Font::TITLE));
      lv_obj_set_pos(title_label_, 0, 0);
    }
    if (list_) {
      const int row_height = head_ + gap_;
      const int count = std::max(1, std::min(kMaxRows, (height_ - title_height_ + gap_) / row_height));
      rows_.resize(count);
      for (int i = 0; i < count; ++i) make_row(rows_[i], c.parent, title_height_ + i * row_height);
      note_ = ui::label(c.parent, Font::BODY, theme::MUTED);
      lv_obj_set_size(note_, width_, ui::line_height(Font::BODY));
      lv_obj_set_pos(note_, 0, height_ - ui::line_height(Font::BODY));
    } else {
      rows_.resize(1);
      make_row(rows_[0], c.parent, title_height_);
      big_ = ui::label(c.parent, Font::VALUE, theme::INK);
      later_ = ui::label(c.parent, Font::BODY, theme::MUTED);
      lv_obj_set_width(big_, width_);
      lv_obj_set_size(later_, width_, ui::line_height(Font::BODY));
      lv_obj_set_pos(later_, 0, height_ - ui::line_height(Font::BODY));
    }
  }

  void on_state(JsonObjectConst data) override {
    departures_.clear();
    wait_ = data["wait"] | "";
    stale_ = data["stale"] | false;
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      Departure departure;
      departure.line = item["line"] | "";
      departure.destination = item["destination"] | "";
      departure.planned = item["planned"] | 0u;
      departure.at = item["estimated"] | (item["planned"] | 0u);
      departure.platform = item["platform"] | "";
      collect_info(item["hints"], departure.info);
      collect_info(item["info_links"], departure.info);
      if (departure.planned && departure.at > departure.planned + 60)
        departure.delay = static_cast<int>((departure.at - departure.planned + 30) / 60);
      departure.cancelled = item["cancelled"] | false;
      if (!departure.at || departure.line.empty() || departure.cancelled) continue;
      if (!lines_.empty() && std::find(lines_.begin(), lines_.end(), departure.line) == lines_.end()) continue;
      if (!direction_a_.empty() && departure.destination != direction_a_ && departure.destination != direction_b_) continue;
      if (!platform_.empty() && departure.platform != platform_) continue;
      departures_.push_back(departure);
    }
    std::sort(departures_.begin(), departures_.end(), [](const Departure &a, const Departure &b) { return a.at < b.at; });
    if (departures_.size() > kMaxRows) departures_.resize(kMaxRows);
    notice_.clear();
    for (const auto &departure : departures_) {
      if (!departure.info.empty()) {
        notice_ = departure.info;
        break;
      }
    }
  }

  void on_tap() override {
    if (!plugin_) return;
    const std::string card_title = title_.empty() ? plugin_->text("details_title") : title_;
    plugin_->set_details(card_title, details_text());
    tessera::open_card(plugin_->plugin_id(), "details", "", -1, card_title);
  }

  void on_tick(uint32_t now) override {
    const char *why = !stop_.empty() ? nullptr : "no_stop";
    if (!why && !now) why = "waiting";
    if (!why && !wait_.empty()) why = "waiting";
    if (!why && departures_.empty()) why = "no_departures";
    if (list_) {
      paint_list(why, now);
    } else {
      paint_single(why, now);
    }
  }

  void on_theme() override {
    for (auto &row : rows_) {
      lv_obj_set_style_bg_color(row.badge, ui::color(theme::ACCENT), 0);
      ui::set_color(row.number, theme::ON_ACCENT);
      ui::set_color(row.destination, list_ ? theme::INK : theme::MUTED);
      if (row.when) ui::set_color(row.when, theme::INK);
    }
    if (title_label_) ui::set_color(title_label_, theme::INK);
    if (big_) ui::set_color(big_, theme::INK);
    if (later_) ui::set_color(later_, theme::MUTED);
    if (note_) ui::set_color(note_, theme::MUTED);
    on_tick(tessera::epoch());
  }

 private:
  static constexpr int kMaxRows = 8;

  struct Row {
    lv_obj_t *badge{};
    lv_obj_t *number{};
    lv_obj_t *destination{};
    lv_obj_t *when{};
    int y = 0;
  };

  LinzDepartures *plugin_;
  int width_ = 0;
  int height_ = 0;
  int head_ = 0;
  int gap_ = 0;
  int title_height_ = 0;
  bool list_ = false;
  bool stale_ = false;
  bool show_platform_ = true;
  bool show_time_ = true;
  std::string stop_;
  std::string title_;
  std::string platform_;
  std::string notice_;
  std::string direction_a_;
  std::string direction_b_;
  std::string wait_;
  std::vector<std::string> lines_;
  std::vector<Departure> departures_;
  std::vector<Row> rows_;
  lv_obj_t *big_{};
  lv_obj_t *later_{};
  lv_obj_t *note_{};
  lv_obj_t *title_label_{};

  void read_options(JsonObjectConst options) {
    for (JsonObjectConst choice : options["line"].as<JsonArrayConst>()) lines_.push_back(choice["value"] | "");
    if (lines_.empty()) {
      const std::string single = options["line"]["value"] | "";
      if (!single.empty()) lines_.push_back(single);
    }
    direction_a_ = options["direction_a"]["value"] | "";
    if (direction_a_.empty()) direction_a_ = options["direction"]["value"] | "";
    direction_b_ = options["direction_b"]["value"] | "";
    platform_ = options["platform"]["value"] | "";
    if (platform_.empty()) platform_ = options["platform"] | "";
    show_platform_ = options["show_platform"] | true;
    show_time_ = options["show_time"] | true;
  }

  void make_row(Row &row, lv_obj_t *parent, int y) {
    row.y = y;
    row.badge = ui::block(parent, theme::ACCENT);
    row.number = ui::label(row.badge, Font::TITLE, theme::ON_ACCENT);
    row.destination = ui::label(parent, Font::BODY, list_ ? theme::INK : theme::MUTED);
    if (list_) {
      row.when = ui::label(parent, Font::TITLE, theme::INK);
      lv_obj_set_style_text_align(row.when, LV_TEXT_ALIGN_RIGHT, 0);
    }
  }

  std::string countdown(uint32_t at, uint32_t now, bool exact = false) const {
    if (at <= now + 30) return plugin_->text("now");
    const uint32_t left = at - now;
    if (exact) {
      char text[16];
      snprintf(text, sizeof(text), "%u:%02u", static_cast<unsigned>(left / 60), static_cast<unsigned>(left % 60));
      return text;
    }
    return tessera::format(plugin_->text("minutes"), static_cast<long>(left / 60));
  }

  std::string when_text(const Departure &departure, uint32_t now, bool exact = false) const {
    const std::string left = countdown(departure.at, now, exact);
    std::string result = show_time_ ? tessera::clock_text(departure.at) + "  " + left : left;
    if (show_platform_ && !departure.platform.empty()) result += "  " + departure.platform;
    if (departure.delay > 0) result += "  " + tessera::format(plugin_->text("delay"), departure.delay);
    return result;
  }

  void place_row(Row &row, const std::string &line, const std::string &destination, const std::string &when) {
    const bool shown = !line.empty();
    const int pad = ui::px(ui::large() ? 8 : 5);
    const int badge_width = shown ? std::min(width_ / 2, ui::text_width(line, Font::TITLE) + 2 * pad) : 0;
    const int x = shown ? badge_width + ui::px(6) : 0;
    int right = width_;
    lv_obj_set_style_opa(row.badge, shown ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    ui::set_text(row.number, line);
    lv_obj_set_size(row.badge, std::max(1, badge_width), head_);
    lv_obj_set_pos(row.badge, 0, row.y);
    lv_obj_center(row.number);
    if (row.when) {
      const int when_width = when.empty() ? 0 : std::min(width_ / 2, ui::text_width(when, Font::TITLE) + ui::px(2));
      ui::set_text(row.when, when);
      lv_obj_set_size(row.when, std::max(1, when_width), ui::line_height(Font::TITLE));
      lv_obj_set_pos(row.when, width_ - when_width, row.y + (head_ - ui::line_height(Font::TITLE)) / 2);
      right = width_ - when_width - (when_width ? ui::px(6) : 0);
    }
    ui::set_text(row.destination, destination);
    lv_obj_set_size(row.destination, std::max(1, right - x), ui::line_height(Font::BODY));
    lv_obj_set_pos(row.destination, x, row.y + (head_ - ui::line_height(Font::BODY)) / 2);
  }

  void paint_list(const char *why, uint32_t now) {
    for (size_t i = 0; i < rows_.size(); ++i) {
      if (i == 0 && why) {
        place_row(rows_[i], "", plugin_->text(why), "");
      } else if (why || i >= departures_.size()) {
        place_row(rows_[i], "", "", "");
      } else {
        place_row(rows_[i], departures_[i].line, departures_[i].destination, when_text(departures_[i], now));
      }
    }
    if (note_) ui::set_text(note_, !notice_.empty() ? notice_ : (stale_ ? plugin_->text("old") : ""));
  }

  void paint_single(const char *why, uint32_t now) {
    Row &row = rows_[0];
    if (why) {
      place_row(row, "", plugin_->text(why), "");
      ui::set_text(big_, "");
      ui::set_text(later_, "");
      return;
    }
    place_row(row, departures_[0].line, departures_[0].destination, "");
    const Departure &next = departures_[0];
    const std::string main_time = show_time_ ? tessera::clock_text(next.at) : countdown(next.at, now, true);
    ui::set_text(big_, main_time);
    const int room = height_ - title_height_ - head_ - ui::line_height(Font::BODY);
    lv_obj_set_height(big_, ui::line_height(Font::VALUE));
    lv_obj_set_pos(big_, 0, title_height_ + head_ + std::max(0, (room - ui::line_height(Font::VALUE)) / 2));
    std::string later = notice_;
    if (later.empty()) {
      later = countdown(next.at, now);
      for (size_t i = 1; i < departures_.size() && i < 3; ++i) {
        later += "  ·  " + countdown(departures_[i].at, now);
      }
    }
    ui::set_text(later_, later);
  }

  std::string details_text() const {
    std::string result;
    for (const auto &departure : departures_) {
      if (!result.empty()) result += "\n\n";
      result += departure.line + "  " + departure.destination + "\n";
      result += when_text(departure, tessera::epoch());
      if (departure.cancelled) result += "  " + std::string(plugin_->text("cancelled"));
      if (!departure.info.empty()) result += "\n" + departure.info;
    }
    if (result.empty()) result = notice_.empty() ? plugin_->text("no_departures") : notice_;
    return result;
  }
};

class DetailsCard : public tessera::Card {
 public:
  explicit DetailsCard(const LinzDepartures *plugin) : plugin_(plugin) {}

  void open(const tessera::CardContext &c) override {
    text_ = ui::text(c.parent, Font::BODY, theme::INK);
    lv_obj_set_size(text_, c.width, c.height);
    lv_obj_set_pos(text_, 0, 0);
    ui::set_text(text_, plugin_->detail_text());
  }

  void on_theme() override {
    if (text_) ui::set_color(text_, theme::INK);
  }

 private:
  const LinzDepartures *plugin_;
  lv_obj_t *text_{};
};

void LinzDepartures::setup() {
  // One adaptive tile, as in Tessera's official public-transport plugin: single-cell cards show the next departure,
  // wider cards become a list automatically.
  add_tile("next", [this]() { return new DeparturesTile(this); });
  add_card("details", [this]() { return new DetailsCard(this); });
}

}  // namespace esphome::linz_departures
