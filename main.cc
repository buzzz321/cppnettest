#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// from https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
template <typename F> struct privDefer {
  F f;
  privDefer(F f) : f(f) {}
  ~privDefer() { f(); }
};

template <typename F> privDefer<F> defer_func(F f) { return privDefer<F>(f); }

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code) auto DEFER_3(_defer_) = defer_func([&]() { code; })
//----------------------------------------------------------------------

using namespace std;
int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  cout << "Starting server" << endl;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == 0) {
    cerr << "couldnt open socket: " << strerror(errno) << endl;
    exit(1);
  }
  defer(close(sockfd));
  int opt = 1;

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    cerr << "couldnt perform setsockopt: " << strerror(errno) << endl;
    exit(1);
  }

  constexpr in_port_t PORT = 7312;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    cerr << "couldnt bind socket: " << strerror(errno) << endl;
    exit(1);
  }
  char buffer[1024] = {0};
  while (true) {
    memset(buffer, 0, 1024);
    if (listen(sockfd, 3) < 0) {
      cerr << "Problem while listening to  socket: " << strerror(errno) << endl;
      exit(1);
    }
    int newSocket{0};
    if ((newSocket = accept(sockfd, (struct sockaddr *)&address,
                            (socklen_t *)&addrlen)) < 0) {
      cerr << "Problem while listening to  socket: " << strerror(errno) << endl;
      exit(1);
    }
    defer(close(newSocket));

    int valRead = read(newSocket, buffer, 1024);
    cout << "Got byte(s) " << valRead << " from client, msg: " << buffer
         << endl;
    send(newSocket, buffer, strlen(buffer), 0);
  }

  return 0;
}
