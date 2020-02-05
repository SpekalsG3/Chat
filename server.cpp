// -lws2_32
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <ws2tcpip.h>
#include <winsock2.h>

#define SERVER_ADDR "127.0.0.1"
// #define SERVER_PORT "27015"
#define SERVER_PORT 27015
#define BUFLEN 1024

class client{
public:
  client(SOCKET sock, std::string nickname, bool ready) {
    this->sock = sock;
    this->nickname = nickname;
    this->ready = ready;
  }

  SOCKET sock;
  std::string nickname;
  bool ready;
};

SOCKET listening;
std::vector<client> clients;

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

  for (const client& cl : clients) {
    if (cl.sock != listening && cl.sock != author && cl.ready)
      send(cl.sock, message.c_str(), message.size(), 0);
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

  fd_set master;
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
      } else {
        char buf[BUFLEN];
        ZeroMemory(buf, BUFLEN);

        int bytesCount = recv(sock, buf, BUFLEN, 0);
        std::vector<std::string> msg = split(std::string(buf));
        if (bytesCount <= 0) {

          for (int i = 0; i < clients.size(); i++) {
            if (clients[i].sock == sock) {
              broadcast(sock, clients[i].nickname + " disconnected");
              clients.erase(clients.begin() + i);
              break;
            }
          }

          closesocket(sock);
          FD_CLR(sock, &master);
        } else if (msg[0] == "conn") {
          bool success = true;
          for (int i = 0; i < clients.size(); i++) {
            if (clients[i].nickname == msg[1]) {
              send(sock, "WR", 2, 0);
              success = false;
              break;
            }
          }
          if (success) {
            std::string greeting = "OKWelcome!";

            send(sock, greeting.c_str(), greeting.size(), 0);
            clients.push_back({sock, msg[1], true});
            broadcast(sock, msg[1] + " connected");
          }
        } else {
          for (int i = 0; i < clients.size(); i++) {
            if (clients[i].sock == sock) {
              broadcast(sock, "<" + clients[i].nickname + "> " + msg[1]);
              break;
            }
          }
        }
      }
    }
  }

  FD_CLR(listening, &master);
  closesocket(listening);

  std::string msg = "Shutting down...";

  while (master.fd_count > 0) {
    SOCKET sock = master.fd_array[0];
    send(sock, msg.c_str(), msg.size(), 0);
    FD_CLR(sock, &master);
    closesocket(sock);
  }

  WSACleanup();

  return 0;
}