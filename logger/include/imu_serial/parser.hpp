#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace imu {

struct Sample {
  std::uint64_t time_us = 0;
  double ax = 0.0;
  double ay = 0.0;
  double az = 0.0;
  double gx = 0.0;
  double gy = 0.0;
  double gz = 0.0;
};

enum class ParseResult {
  kSample,
  kIgnored,
  kMalformed,
};

std::string_view header();

ParseResult parseLine(std::string_view line, Sample& sample);

std::string formatSample(const Sample& sample);

}  // namespace imu
