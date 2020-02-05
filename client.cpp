// -lws2_32
#include <iostream>
#include <string>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <thread>
#include <conio.h>

#define BUFLEN 1024

SOCKET sock;
std::string msg = "";

struct modifiers {
  bool insert = false;
} mods;

void recvHandler() {
  char buf[BUFLEN];
  while (true) {
    ZeroMemory(buf, BUFLEN);
    int bytesCount = recv(sock, buf, BUFLEN, 0);
    if (bytesCount > 0) {
      std::cout << "\r" << std::string(buf, 0, bytesCount);

      if (msg.length() > bytesCount) {
        std::cout << std::string(msg.length() - bytesCount + 2, ' ');
      }

      std::cout << std::endl << "> " << msg;
    }
  }
}

int main() {
  std::string ip;
  int port = 27015;

  std::cout << "Enter Server IP (127.0.0.1): ";
  std::getline(std::cin, ip);

  if (ip.size() == 0)
    ip = "127.0.0.1";

  WSAData wsaData;
  int iRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iRes != 0) {
    std::cout << "WSAStartup failed with error: " << iRes << std::endl;
    std::cin.get();
    return 1;
  }

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET) {
    std::cout << "socket creaton failed with error: " << WSAGetLastError() << std::endl;
    WSACleanup();
    std::cin.get();
    return 1;
  }

  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  hint.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

  iRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
  if (iRes == SOCKET_ERROR) {
    std::cout << "connect failed with error: " << WSAGetLastError() << std::endl;
    closesocket(sock);
    WSACleanup();
    std::cin.get();
    return 1;
  }

  std::string nickname = "";

  while (true) {
    std::cout << "Enter your nickname: ";
    std::cin >> nickname;

    std::string sending = "conn:" + nickname;

    iRes = send(sock, sending.c_str(), sending.length(), 0);
    if (iRes == SOCKET_ERROR)
      std::cout << "\r---Error # " + std::to_string(WSAGetLastError()) + ": Failed to send this message---" << std::endl;
    else {
      char buf[2];
      ZeroMemory(buf, 2);
      int bytesCount = recv(sock, buf, 2, 0);
      if (bytesCount <= 0)
        std::cout << "\r---Error # " + std::to_string(WSAGetLastError()) + ": Failed to recv this message---" << std::endl;
      else {
        if (buf[0] == 'O' && buf[1] == 'K')
          break;
        else
          std::cout << "This nickname is picked. Choose another one" << std::endl;
      }
    }
  }

  std::thread recvThr(recvHandler);
  recvThr.detach();

  int ch;
  int x = -1;
  std::string tmp;
  std::cout << "> ";

  while (true) {
    ch = getch();

    if (ch == 224 && kbhit()) { // Special Key
      switch(getch()) {
        case 75: // Arrow left
          if (x > -1) {
            x--;
            std::cout << "\b";
          }
          break;
        case 72: // Arrow up
          break;
        case 77: // Arrow right
          if (x + 1 < msg.length()) {
            x++;
            std::cout << msg[x];
          }
          break;
        case 80: // Arrow bottom
          break;
        case 82: // Insert
          mods.insert = !mods.insert;
          break;
        case 83: // Delete
          if (x + 1< msg.length()) {
            msg.erase(x+1, 1);
            std::cout << msg.substr(x + 1) << ' ' << std::string(msg.length() - x, '\b');
          }
          break;
        case 71: // Home
          if (x > -1) {
            std::cout << std::string(x + 1, '\b');
            x = -1;
          }
          break;
        case 79: // End
          if (x + 1 < msg.length()) {
            std::cout << msg.substr(x+1);
            x = msg.length()-1;
          }
          break;
      }
    } else if ((31 < ch && ch < 91) || (96 < ch && ch < 123)    // English
      ||(127 < ch && ch < 176) || (223 < ch && 242)) {  // Russian
      x++;
      if (mods.insert) {
        std::cout << static_cast<char>(ch);
      } else {
        tmp = msg.substr(x);
        std::cout << static_cast<char>(ch);
        std::cout << tmp << std::string(tmp.length(), '\b');
      }
      msg.insert(x, 1, static_cast<char>(ch));
    } else if (ch == 8 && x > -1) { // Backspace
      std::cout << "\b";
      msg.erase(x, 1);
      tmp = msg.substr(x);
      std::cout << tmp << ' ' << std::string(tmp.length()+1, '\b');
      x--;
    } else if (ch == 13) {  // Enter
      std::string sending = "chat:" + msg;
      iRes = send(sock, sending.c_str(), sending.length(), 0);
      if (iRes == SOCKET_ERROR)  {

        std::string errMsg = "\r---Error # " + std::to_string(WSAGetLastError()) + ": Failed to send this message---";
        std::cout << errMsg;

        if (msg.length() > errMsg.length())
          std::cout << std::string(msg.length()-errMsg.length(), ' ');

        std::cout << std::endl << "> " << msg;
      } else {
        x = -1;
        std::cout << "\r<you> " << msg << std::endl << "> ";
        msg.clear();
      }
    }
  }

  closesocket(sock);
  WSACleanup();
  return 1;
}