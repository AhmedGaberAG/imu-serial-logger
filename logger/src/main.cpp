#include "imu_serial/parser.hpp"
#include "imu_serial/serial_port.hpp"

#include <cerrno>
#include <charconv>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

#include <unistd.h>

namespace {

volatile std::sig_atomic_t g_stop = 0;

void handleSignal(int) { g_stop = 1; }

struct Options {
  std::string port;
  std::string output = "-";
  int baud = 115200;
  bool help = false;
};

void printUsage(std::ostream& out) {
  out << "Usage: imu_logger --port <device> [--out <file>] [--baud <rate>]\n"
         "\n"
         "  -p, --port <device>  serial device, e.g. /dev/ttyUSB0 (required)\n"
         "  -o, --out <file>     output CSV file, '-' for stdout (default)\n"
         "  -b, --baud <rate>    serial baud rate (default 115200)\n"
         "  -h, --help           show this help\n";
}

bool parseArgs(int argc, char** argv, Options& options, std::string& error) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto next = [&](std::string& value) {
      if (i + 1 >= argc) {
        error = "missing value for " + arg;
        return false;
      }
      value = argv[++i];
      return true;
    };

    if (arg == "-h" || arg == "--help") {
      options.help = true;
    } else if (arg == "-p" || arg == "--port") {
      if (!next(options.port)) {
        return false;
      }
    } else if (arg == "-o" || arg == "--out") {
      if (!next(options.output)) {
        return false;
      }
    } else if (arg == "-b" || arg == "--baud") {
      std::string value;
      if (!next(value)) {
        return false;
      }
      int baud = 0;
      const auto result =
          std::from_chars(value.data(), value.data() + value.size(), baud);
      if (result.ec != std::errc() ||
          result.ptr != value.data() + value.size() || baud <= 0) {
        error = "invalid baud rate: " + value;
        return false;
      }
      options.baud = baud;
    } else {
      error = "unknown argument: " + arg;
      return false;
    }
  }

  if (!options.help && options.port.empty()) {
    error = "--port is required";
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  Options options;
  std::string error;
  if (!parseArgs(argc, argv, options, error)) {
    std::cerr << "imu_logger: " << error << "\n\n";
    printUsage(std::cerr);
    return 2;
  }
  if (options.help) {
    printUsage(std::cout);
    return 0;
  }

  imu::SerialPort port;
  if (!port.open(options.port, options.baud, error)) {
    std::cerr << "imu_logger: " << error << '\n';
    return 1;
  }

  std::ofstream file;
  std::ostream* output = &std::cout;
  if (options.output != "-") {
    file.open(options.output, std::ios::out | std::ios::trunc);
    if (!file) {
      std::cerr << "imu_logger: cannot open output file: " << options.output
                << '\n';
      return 1;
    }
    output = &file;
  }

  *output << imu::header() << '\n';
  output->flush();
  if (!*output) {
    std::cerr << "imu_logger: failed to write header\n";
    return 1;
  }

  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  std::string buffer;
  std::uint64_t samples = 0;
  std::uint64_t malformed = 0;
  std::uint64_t first_time_us = 0;
  std::uint64_t last_time_us = 0;

  while (!g_stop) {
    char chunk[4096];
    const ssize_t count = ::read(port.fd(), chunk, sizeof(chunk));
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "imu_logger: read failed: " << std::strerror(errno) << '\n';
      break;
    }
    if (count == 0) {
      continue;
    }

    buffer.append(chunk, static_cast<std::size_t>(count));
    std::size_t newline = 0;
    while ((newline = buffer.find('\n')) != std::string::npos) {
      const std::string_view line(buffer.data(), newline);
      imu::Sample sample;
      const imu::ParseResult result = imu::parseLine(line, sample);
      if (result == imu::ParseResult::kSample) {
        if (samples == 0) {
          first_time_us = sample.time_us;
        }
        last_time_us = sample.time_us;
        *output << imu::formatSample(sample) << '\n';
        ++samples;
      } else if (result == imu::ParseResult::kMalformed) {
        ++malformed;
      }
      buffer.erase(0, newline + 1);
    }

    if (buffer.size() > 64 * 1024) {
      buffer.clear();
      ++malformed;
    }

    output->flush();
    if (!*output) {
      std::cerr << "imu_logger: failed to write output\n";
      break;
    }
  }

  output->flush();
  const double seconds = samples > 0
                             ? (last_time_us - first_time_us) / 1'000'000.0
                             : 0.0;
  std::cerr << "imu_logger: wrote " << samples << " samples";
  if (seconds > 0.0) {
    std::cerr << " over " << seconds << " s";
  }
  if (malformed > 0) {
    std::cerr << ", ignored " << malformed << " malformed lines";
  }
  std::cerr << '\n';
  return samples > 0 ? 0 : 1;
}
