#include "imu_serial/serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace imu {
namespace {

bool baudToSpeed(int baud, speed_t& speed) {
  switch (baud) {
    case 9600:
      speed = B9600;
      return true;
    case 19200:
      speed = B19200;
      return true;
    case 38400:
      speed = B38400;
      return true;
    case 57600:
      speed = B57600;
      return true;
    case 115200:
      speed = B115200;
      return true;
    case 230400:
      speed = B230400;
      return true;
#ifdef B460800
    case 460800:
      speed = B460800;
      return true;
#endif
#ifdef B921600
    case 921600:
      speed = B921600;
      return true;
#endif
    default:
      return false;
  }
}

}  // namespace

SerialPort::~SerialPort() { close(); }

SerialPort::SerialPort(SerialPort&& other) noexcept : fd_(other.fd_) {
  other.fd_ = -1;
}

SerialPort& SerialPort::operator=(SerialPort&& other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

bool SerialPort::open(const std::string& path, int baud, std::string& error) {
  speed_t speed = 0;
  if (!baudToSpeed(baud, speed)) {
    error = "unsupported baud rate: " + std::to_string(baud);
    return false;
  }

  close();
  fd_ = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd_ < 0) {
    error = "cannot open " + path + ": " + std::strerror(errno);
    return false;
  }

  struct termios settings {};
  if (tcgetattr(fd_, &settings) != 0) {
    error = "tcgetattr failed: " + std::string(std::strerror(errno));
    close();
    return false;
  }

  cfmakeraw(&settings);
  settings.c_cflag |= (CLOCAL | CREAD);
  settings.c_cflag &= ~(CSTOPB | PARENB | CRTSCTS);
  settings.c_cflag &= ~CSIZE;
  settings.c_cflag |= CS8;
  settings.c_cc[VMIN] = 0;
  settings.c_cc[VTIME] = 5;

  if (cfsetispeed(&settings, speed) != 0 ||
      cfsetospeed(&settings, speed) != 0 ||
      tcsetattr(fd_, TCSANOW, &settings) != 0) {
    error = "cannot configure " + path + ": " + std::strerror(errno);
    close();
    return false;
  }

  tcflush(fd_, TCIOFLUSH);
  return true;
}

void SerialPort::close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool SerialPort::isOpen() const { return fd_ >= 0; }

int SerialPort::fd() const { return fd_; }

}  // namespace imu
