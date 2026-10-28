#include "imu_serial/parser.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++g_failures;
  }
}

bool nearlyEqual(double lhs, double rhs) {
  return std::abs(lhs - rhs) < 1e-9;
}

void testValidLine() {
  imu::Sample sample;
  const auto result = imu::parseLine(
      "123456,0.01,-0.02,1.00,1.5,-2.5,0.125", sample);
  check(result == imu::ParseResult::kSample, "valid line is parsed");
  check(sample.time_us == 123456, "timestamp is parsed");
  check(nearlyEqual(sample.ax, 0.01), "ax is parsed");
  check(nearlyEqual(sample.ay, -0.02), "ay is parsed");
  check(nearlyEqual(sample.az, 1.0), "az is parsed");
  check(nearlyEqual(sample.gx, 1.5), "gx is parsed");
  check(nearlyEqual(sample.gy, -2.5), "gy is parsed");
  check(nearlyEqual(sample.gz, 0.125), "gz is parsed");
}

void testWhitespaceAndCrlf() {
  imu::Sample sample;
  const auto result =
      imu::parseLine("  42, 1, 2, 3, 4, 5, 6 \r", sample);
  check(result == imu::ParseResult::kSample, "crlf and spaces are accepted");
  check(sample.time_us == 42, "timestamp with spaces is parsed");
  check(nearlyEqual(sample.gz, 6.0), "last field with spaces is parsed");
}

void testIgnoredLines() {
  imu::Sample sample;
  check(imu::parseLine("", sample) == imu::ParseResult::kIgnored,
        "empty line is ignored");
  check(imu::parseLine("   ", sample) == imu::ParseResult::kIgnored,
        "blank line is ignored");
  check(imu::parseLine("# t_us,ax_g", sample) ==
            imu::ParseResult::kIgnored,
        "comment is ignored");
  check(imu::parseLine("t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps",
                       sample) == imu::ParseResult::kIgnored,
        "uncommented header is ignored");
}

void testMalformedLines() {
  imu::Sample sample;
  check(imu::parseLine("1,2,3", sample) == imu::ParseResult::kMalformed,
        "too few fields is malformed");
  check(imu::parseLine("1,2,3,4,5,6,7,8", sample) ==
            imu::ParseResult::kMalformed,
        "too many fields is malformed");
  check(imu::parseLine("x,2,3,4,5,6,7", sample) ==
            imu::ParseResult::kMalformed,
        "non-numeric timestamp is malformed");
  check(imu::parseLine("-1,2,3,4,5,6,7", sample) ==
            imu::ParseResult::kMalformed,
        "negative timestamp is malformed");
  check(imu::parseLine("1,2,3,4,5,6,nan", sample) ==
            imu::ParseResult::kMalformed,
        "nan is malformed");
  check(imu::parseLine("1,2,3,4,5,6,7,", sample) ==
            imu::ParseResult::kMalformed,
        "trailing comma is malformed");
}

void testFormatRoundTrip() {
  imu::Sample sample;
  sample.time_us = 987654321;
  sample.ax = 0.125;
  sample.ay = -1.5;
  sample.az = 2.25;
  sample.gx = 10.5;
  sample.gy = -20.25;
  sample.gz = 0.0;

  imu::Sample parsed;
  const auto result = imu::parseLine(imu::formatSample(sample), parsed);
  check(result == imu::ParseResult::kSample, "formatted line parses");
  check(parsed.time_us == sample.time_us, "time survives round trip");
  check(nearlyEqual(parsed.ax, sample.ax), "ax survives round trip");
  check(nearlyEqual(parsed.ay, sample.ay), "ay survives round trip");
  check(nearlyEqual(parsed.gz, sample.gz), "gz survives round trip");
}

}  // namespace

int main() {
  testValidLine();
  testWhitespaceAndCrlf();
  testIgnoredLines();
  testMalformedLines();
  testFormatRoundTrip();

  if (g_failures != 0) {
    std::cerr << g_failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all parser tests passed\n";
  return 0;
}
