#include "imu_serial/parser.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <system_error>

namespace imu {
namespace {

std::string_view trim(std::string_view text) {
  constexpr std::string_view kWhitespace = " \t\r\n";
  const auto first = text.find_first_not_of(kWhitespace);
  if (first == std::string_view::npos) {
    return {};
  }
  const auto last = text.find_last_not_of(kWhitespace);
  return text.substr(first, last - first + 1);
}

bool parseUint64(std::string_view text, std::uint64_t& value) {
  text = trim(text);
  if (text.empty()) {
    return false;
  }
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  return result.ec == std::errc() && result.ptr == end;
}

bool parseDouble(std::string_view text, double& value) {
  text = trim(text);
  if (text.empty()) {
    return false;
  }
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return false;
  }
  return std::isfinite(value);
}

}  // namespace

std::string_view header() {
  return "t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps";
}

ParseResult parseLine(std::string_view line, Sample& sample) {
  line = trim(line);
  if (line.empty() || line.front() == '#') {
    return ParseResult::kIgnored;
  }
  if (line.size() >= 4 && line.compare(0, 4, "t_us") == 0 &&
      (line.size() == 4 || line[4] == ',')) {
    return ParseResult::kIgnored;
  }

  std::string_view fields[7];
  std::size_t field_count = 0;
  std::size_t start = 0;
  while (start <= line.size() && field_count < 7) {
    const auto comma = line.find(',', start);
    if (comma == std::string_view::npos) {
      fields[field_count++] = line.substr(start);
      start = line.size() + 1;
    } else {
      fields[field_count++] = line.substr(start, comma - start);
      start = comma + 1;
    }
  }
  if (start <= line.size() || field_count != 7) {
    return ParseResult::kMalformed;
  }

  Sample parsed;
  if (!parseUint64(fields[0], parsed.time_us) ||
      !parseDouble(fields[1], parsed.ax) ||
      !parseDouble(fields[2], parsed.ay) ||
      !parseDouble(fields[3], parsed.az) ||
      !parseDouble(fields[4], parsed.gx) ||
      !parseDouble(fields[5], parsed.gy) ||
      !parseDouble(fields[6], parsed.gz)) {
    return ParseResult::kMalformed;
  }

  sample = parsed;
  return ParseResult::kSample;
}

std::string formatSample(const Sample& sample) {
  char buffer[256];
  const int written = std::snprintf(
      buffer, sizeof(buffer), "%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
      static_cast<unsigned long long>(sample.time_us), sample.ax, sample.ay,
      sample.az, sample.gx, sample.gy, sample.gz);
  if (written <= 0) {
    return {};
  }
  return std::string(buffer, static_cast<std::size_t>(written));
}

}  // namespace imu
