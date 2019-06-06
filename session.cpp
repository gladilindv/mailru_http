#include "session.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>

namespace util {
struct in_addr inaddr(const std::string& aHost) {
  struct in_addr in {0};
  if (aHost.empty()) {
    return in;
  }
  if (inet_pton(AF_INET, aHost.c_str(), &in) == 1) {
    return in;
  }
  struct addrinfo* res = 0;
  auto ret = getaddrinfo(aHost.c_str(), 0, 0, &res);
  if (ret) {
    throw "getaddrinfo failed. Exiting!";
  }
  for (struct addrinfo* addr = res; addr; addr = addr->ai_next) {
    struct sockaddr_in* sin = (struct sockaddr_in*)addr->ai_addr;
    if (sin->sin_addr.s_addr) {
      in = sin->sin_addr;
      freeaddrinfo(res);
      return in;
    }
  }
  freeaddrinfo(res);
  assert(!"no addresses found");
  return in;
}

bool isSocketCloseError(int error) {
  switch (error) {
    case EPIPE:
    case ENETRESET:
    case ECONNABORTED:
    case ECONNRESET:
    case ESHUTDOWN:
      return true;
    default:
      return false;
  }
}
}  // namespace util


bool Session::init(const std::string& aHost, unsigned short aPort) {
  struct sockaddr_in address {0};

  if (!(mSock = socket(AF_INET, SOCK_STREAM, 0))) {
    std::cout << "Failed to create a socket. Exiting!"<< std::endl;
    return false;
  }
  if (setNonBlocking(true) == -1) {
    std::cout << "Failed to set async mode. Exiting!" << std::endl;
    return false;
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(aPort);
  address.sin_addr = util::inaddr(aHost);

  if (connect(mSock, (struct sockaddr*)&address, sizeof(address)) < 0) {
    if (errno != EINPROGRESS) {
      std::cout << "Failed to connect. Exiting!" << std::endl;
      return false;
    }
  }

  fd_set fdset;
  struct timeval timeout;

  FD_ZERO(&fdset);
  FD_SET(mSock, &fdset);
  timeout.tv_sec = 1;  // after 1.5 seconds connect() will timeout
  timeout.tv_usec = 500000;

  auto rc = select(mSock + 1, nullptr, &fdset, nullptr, &timeout);
  if (rc == -1) {
    std::cout << "Failed to select. Exiting!" << std::endl;
    return false;
  }
  if (rc == 0) {
    std::cout << "Timeout. Exiting!" << std::endl;
    return false;
  }

  int so_error;
  socklen_t len;
  getsockopt(mSock, SOL_SOCKET, SO_ERROR, &so_error, &len);
  if (so_error != 0) {
    std::cout << "Connection can`t established. Exiting!" << std::endl;
    return false;
  }

  return true;
}

Session::~Session() { ::close(mSock); }

void Session::sendRequest(const std::string& aHost, uint16_t aPort,
                          const std::string& aPath) {
  std::stringstream ss;
  ss << "GET" << ' ' << aPath << " HTTP/1.1\r\n";
  ss << "Host" << ": " << aHost << ":" << aPort << "\r\n";
  ss << "Content-Length" << ": 0\r\n";
  ss << "Connection" << ": close\r\n";
  ss << "\r\n";
  const auto req = ss.str();
  const char* buf = req.c_str();

  size_t size_send = 0;
  size_t size_left = req.size();
  while (size_left > 0) {
    if ((size_send = send(mSock, buf, size_left, 0)) == -1) {
      std::cout << "send error: " << std::endl;
      throw "Write error. Exiting!";
    }
    size_left -= size_send;
    buf += size_send;
  }
}

void Session::readAndSave(const std::string& aName) {
  if (readHeaders()) 
    saveFile(aName);
}

bool Session::readHeaders() {
  std::vector<char> buffer(4 * 1024);

  while (true) {
    auto ret = recv(mSock, &buffer[0], buffer.size(), MSG_PEEK);

    if (ret == -1) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        usleep(10000);
        continue;
      }
      return false; // remote close connection
    }
    if (ret == 0) {
      return false;
    }

    std::string buf(&buffer[0], ret);
    auto pos = buf.find("\r\n\r\n");
    if (pos == std::string::npos) {
      std::cout << "HTTP header bad format" << std::endl;
      std::cout << buf << std::endl;
      return false;
    }

    int headers_len = static_cast<int>(pos + 4);

    // now get headers with the obtained size from socket
    if ((ret = recv(mSock, &buffer[0], headers_len, 0)) == -1) {
      std::cout << "recv error: " << strerror(errno) << std::endl;
      return false;
    }

    if(buf.find("200 OK") != 8) { // length of "HTTP/1.1 "
      std::cout << "HTTP status is not 200" << std::endl;
      std::cout << buf << std::endl;
      return false;
    }

    break;
  }
  
  return true;
}

void Session::saveFile(const std::string& aName) {
  std::ofstream outfile(aName, std::ofstream::binary);

  std::vector<char> buffer(16 * 1024);
  unsigned total{0};
  
  // track time
  auto start = std::chrono::system_clock::now();
  auto current = start;

  while (true) {
    ssize_t ret = recv(mSock, &buffer[0], buffer.size(), 0);
    if (ret == 0) {
      break;
    }
    if (ret < 0) {
      if (util::isSocketCloseError(errno)) {
        throw "SocketCloseException";  // Closed remotely
      } else if (EAGAIN != errno) {
        throw "SystemError";
      }
    } else {
      outfile.write(&buffer[0], ret);
      total += ret;

      auto end = std::chrono::system_clock::now();
      int elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - current).count();

      if(elapsed_ms > 1000){
        current = end;
        int elapsed_sec =
          std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        
        std::cout << total << " bytes at " <<  total / 1024.0 / elapsed_sec << "KB/s" << std::endl;
      }
    }
  }

  outfile.close();

  // debug stat
  int cnt = 0;
  while(total > 1024){
    total /= 1024.0;
    cnt++;
  }
  std::vector<std::string> dim {"Bytes", "KB", "MB", "GB", "TB", "PB"};
  std::cout << "TOTAL: " << std::to_string(total) << dim[cnt] << std::endl;
  // debug stat
}
