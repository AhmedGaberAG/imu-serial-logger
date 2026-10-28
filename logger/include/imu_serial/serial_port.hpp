#pragma once

#include <string>

namespace imu {

class SerialPort {
 public:
  SerialPort() = default;
  ~SerialPort();

  SerialPort(const SerialPort&) = delete;
  SerialPort& operator=(const SerialPort&) = delete;
  SerialPort(SerialPort&& other) noexcept;
  SerialPort& operator=(SerialPort&& other) noexcept;

  bool open(const std::string& path, int baud, std::string& error);
  void close();
  bool isOpen() const;
  int fd() const;

 private:
  int fd_ = -1;
};

}  // namespace imu
