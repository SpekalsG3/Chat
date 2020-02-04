// -lws2_32
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <ws2tcpip.h>
#include <winsock2.h>

#define SERVER_ADDR "127.0.0.1"
// #define SERVER_PORT "27015"
#define SERVER_PORT 27015
#define BUFLEN 1024

fd_set master;
SOCKET listening;

std::vector<std::string> split(std::string msg) {
  std::stringstream splitting(msg);
  std::string segment;
  std::vector<std::string> seglist;

  while(std::getline(splitting, segment, ':'))
   seglist.push_back(segment);

  return seglist;
}

void broadcast(SOCKET author, std::string message) {
  std::cout << message << std::endl;

  for (int i = 0; i < master.fd_count; i++) {
    SOCKET out = master.fd_array[i];

    if (out != listening && out != author)
      send(out, message.c_str(), message.size() + 1, 0);
  }
}

int main() {
  WSADATA wsaData;
  int iRes = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iRes != 0) {
    std::cout << "WSAStartup failed with error: " << iRes << std::endl;
    std::cin.get();
    return 1;
  }

  std::vector<std::pair<SOCKET, std::string> > nicknames;

  struct addrinfo hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Resolving the server address and port
  struct addrinfo *result = nullptr;
  iRes = getaddrinfo(SERVER_ADDR, std::to_string(SERVER_PORT).c_str(), &hints, &result);
  if (iRes != 0) {
    std::cout << "getaddrinfo failed with error: " << iRes << std::endl;
    WSACleanup();
    std::cin.get();
    return 1;
  }

  listening = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (listening == INVALID_SOCKET) {
    std::cout << "Unable to create a socket" << std::endl;
    std::cin.get();
    return 1;
  }
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(SERVER_PORT);
  hint.sin_addr.S_un.S_addr = INADDR_ANY;

  bind(listening, (sockaddr*)&hint, sizeof(hint));

  listen(listening, SOMAXCONN);

  FD_ZERO(&master);
  FD_SET(listening, &master);

  std::cout << "Started server" << std::endl;
  bool running = true;
  while (running) {
    fd_set copy = master;

    int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

    for (int i = 0; i < socketCount; i++) {
      SOCKET sock = copy.fd_array[i];

      if (sock == listening) {
        SOCKET client = accept(listening, nullptr, nullptr);
        FD_SET(client, &master);
        std::string greeting = "Welcome!\r";
        send(client, greeting.c_str(), greeting.size() + 1, 0);
      } else {
        char buf[BUFLEN];
        ZeroMemory(buf, BUFLEN);

        int bytesCount = recv(sock, buf, BUFLEN, 0);
        std::vector<std::string> msg = split(std::string(buf));
        if (bytesCount <= 0) {

          for (int i = 0; i < nicknames.size(); i++) {
            if (nicknames[i].first == sock) {
              broadcast(sock, nicknames[i].second + " disconnected");
              nicknames.erase(nicknames.begin() + i);
              break;
            }
          }

          closesocket(sock);
          FD_CLR(sock, &master);
        } else if (msg[0] == "conn") {
          nicknames.push_back(std::make_pair(sock, msg[1]));
          broadcast(sock, msg[1] + " connected");
        } else {
          broadcast(sock, "<" + msg[0] + "> " + msg[1]);
        }
      }
    }
  }

  FD_CLR(listening, &master);
  closesocket(listening);

  std::string msg = "Shutting down...";

  while (master.fd_count > 0) {
    SOCKET sock = master.fd_array[0];
    send(sock, msg.c_str(), msg.size() + 1, 0);
    FD_CLR(sock, &master);
    closesocket(sock);
  }

  WSACleanup();

  return 0;
}