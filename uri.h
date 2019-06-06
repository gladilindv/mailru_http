#include <string>

class Uri {
 public:
  Uri(const std::string& uri)
      : mPort(80), mHost("mail.ru"), mPath("/"), mScheme("http") {
    parse(uri);
  }

  const uint16_t port() { return mPort; }
  const std::string& host() { return mHost; }
  const std::string& path() { return mPath; }
  const std::string& scheme() const { return mScheme; } 

 private:
  uint16_t mPort;
  std::string mHost;
  std::string mPath;
  std::string mScheme;

  void parse(const std::string& uri);
};