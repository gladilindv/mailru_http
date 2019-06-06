#include <string>

#include <fcntl.h>

class Session {
  int mSock;

  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;

  Session(Session&& sock) = delete;
  Session& operator=(Session&&) = delete;

  inline int setNonBlocking(bool nonblocking) {
    auto flags = fcntl(mSock, F_GETFL, 0);
    return fcntl(mSock, F_SETFL,
          nonblocking ? (flags | O_NONBLOCK) : (flags & (~O_NONBLOCK)));
  }

 public:
  Session() = default;
  ~Session();

  bool init(const std::string& aHost, unsigned short aPort);
  void sendRequest(const std::string& aHost, uint16_t aPort,
                   const std::string& aPath);
  void readAndSave(const std::string&);

 private:
  bool readHeaders();
  void saveFile(const std::string&);

};
