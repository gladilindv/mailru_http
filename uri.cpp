#include "uri.h"

#include <iostream>
#include <algorithm>
#include <string_view>

void Uri::parse(const std::string& uri) {
  auto it = uri.begin();
  auto end = uri.end();
  if (it == end) {
    std::cout << "Empty uri. exit" << std::endl;
    return;
  }

  const std::string_view prot_end("://");
  auto proto_it = std::search(it, end, prot_end.begin(), prot_end.end());
  if (proto_it == end) {
    // cout << "no scheme founded" << endl;
  } else {
    mScheme = std::string(it, proto_it);
    // std::cout << scheme << std::endl;
    it = proto_it + 3;
  }

  if (it == end) {
    // cout << "Broken uri (1). exit" << endl;
    return;
  }

  auto auth_it = std::find(it, end, '@');
  if (auth_it == end) {
    // cout << "no auth founded" << endl;
  } else {
    auto auth = std::string(it, auth_it);
    // std::cout << auth << std::endl;
    it = auth_it + 1;
  }

  if (it == end) {
    // cout << "Broken uri (2). exit" << endl;
    return;
  }

  mHost = std::string(it, end);

  auto port_it = std::find(it, end, ':');
  auto path_it = std::find(it, end, '/');
  auto data_it = std::find(it, end, '?');

  if (path_it != end && path_it < data_it) {
    mHost = std::string(it, path_it);
    mPath = std::string(path_it, end);
  } else {
    if (data_it != end) {
      mHost = std::string(it, data_it);
      mPath = "/" + std::string(data_it, end);
    } else {
      // cout << "no path founded, use root" << endl;
    }
  }

  if (port_it != end && port_it < path_it && port_it < data_it) {
    mHost = std::string(it, port_it);
    auto min_it = std::min(path_it, data_it);
    auto portS = std::string(++port_it, min_it);
    mPort = std::stoi(portS);
  } else {
    // cout << "no port founded, use default 80" << endl;
  }

  // std::cout << mHost << "|" << mPort << "|" << mPath << std::endl;
}