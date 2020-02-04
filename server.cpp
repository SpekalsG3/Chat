// -lws2_32
#include <iostream>
#include <sstream>
#include <string>
// #include <map>
#include <ws2tcpip.h>
#include <winsock2.h>

#define SERVER_ADDR "127.0.0.1"
// #define SERVER_PORT "27015"
#define SERVER_PORT 27015
#define BUFLEN 1024

int main() {
  WSADATA wsaData;
  int iRes = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iRes != 0) {
    std::cout << "WSAStartup failed with error: " << iRes << std::endl;
    std::cin.get();
    return 1;
  }

  // std::map<SOCKET, std::string> nicknames;

  struct addrinfo hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // struct addrinfo *result = nullptr;
  // iRes = getaddrinfo(SERVER_ADDR, SERVER_PORT, &hints, &result);
  // if (iRes != 0) {
  //   std::cout << "getaddrinfo failed with error: " << iRes << std::endl;
  //   WSACleanup();
  //   std::cin.get();
  //   return 1;
  // }

  // SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
  // if (listening == INVALID_SOCKET) {
  //   std::cout << "Unable to create a socket" << std::endl;
  //   std::cin.get();
  //   return 1;
  // }

  // bind(listening, &hints.ai_addr, sizeof(hints.ai_addr));

  SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(SERVER_PORT);
  hint.sin_addr.S_un.S_addr = INADDR_ANY;

  bind(listening, (sockaddr*)&hint, sizeof(hint));

  listen(listening, SOMAXCONN);

  fd_set master;
  FD_ZERO(&master);
  FD_SET(listening, &master);

  bool running = true;
  while (running) {
    fd_set copy = master;

    int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

    for (int i = 0; i < socketCount; i++) {
      SOCKET sock = copy.fd_array[i];

      if (sock == listening) {
        SOCKET client = accept(listening, nullptr, nullptr);

        // char nickname[BUFLEN];
        // ZeroMemory(nickname, BUFLEN);
        // int bytesCount = recv(sock, nickname, BUFLEN, 0);

        // if (bytesCount == 0) {
        //   closesocket(client);
        // } else {
          FD_SET(client, &master);
          // map.insert(pair<SOCKET, std::string>());

          std::string greeting = "Welcome!\r";
          send(client, greeting.c_str(), greeting.size() + 1, 0);
        // }
      } else {
        char buf[BUFLEN];
        ZeroMemory(buf, BUFLEN);

        int bytesCount = recv(sock, buf, BUFLEN, 0);
        if (bytesCount == 0) {
          closesocket(sock);
          FD_CLR(sock, &master);
        } else {
          for (int i = 0; i < master.fd_count; i++) {
            SOCKET out = master.fd_array[i];
            if (out != listening && out != sock) {
              std::ostringstream ss;
              ss << "<" << sock << "> " << buf << "\r";

              std::string msg = ss.str();
              std::cout << msg << std::endl;
              send(out, msg.c_str(), msg.size() + 1, 0);
            }
          }
        }
      }
    }
  }
  return 0;
}