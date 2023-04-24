#include <iostream>
#include <string>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include "spdlog/spdlog.h"

#define MAX_CLIENT 100
#define BUFFER_LEN 1024

using namespace std;

struct UserInfo {
  int socket;
  string nickname;
};

map<string, UserInfo> user_list;
mutex user_list_mutex;

struct Message {
  string from;
  string to;
  string content;
  chrono::system_clock::time_point timestamp;
};

struct CompareMessage {
  bool operator()(const Message& m1, const Message& m2) {
    return m1.timestamp > m2.timestamp;
  }
};

priority_queue<Message, vector<Message>, CompareMessage> message_queue;
mutex message_queue_mutex; condition_variable message_queue_cv;
void add_user(const string& nickname, const UserInfo& info)
{
  lock_guard<mutex> lock(user_list_mutex);
  user_list[nickname] = info;
}

void remove_user(const string& nickname)
{
  lock_guard<mutex> lock(user_list_mutex);
  user_list.erase(nickname);
}

UserInfo get_user_info(const string& nickname)
{
  lock_guard<mutex> lock(user_list_mutex);
  return user_list[nickname];
}

void add_message(Message message)
{
  lock_guard<mutex> lock(message_queue_mutex);
  message_queue.push(message);
  message_queue_cv.notify_one();
}

Message get_message()
{
  unique_lock<mutex> lock(message_queue_mutex);
  while (message_queue.empty()) {
    message_queue_cv.wait(lock);
  }

  Message message = message_queue.top();
  message_queue.pop();

  return message;
}

void handle_client(int client_socket)
{
  char buffer[BUFFER_LEN];
  int bytes_received = recv(client_socket, buffer, BUFFER_LEN, 0);
  if (bytes_received <= 0)
    return;

  buffer[bytes_received] = '\0';
  string nickname = buffer;
  UserInfo user_info;
  user_info.nickname = nickname;
  user_info.socket = client_socket;

  add_user(user_info.nickname, user_info);

  while (true) {
    try {
      bytes_received = recv(client_socket, buffer, BUFFER_LEN, 0);
      if (bytes_received <= 0)
        break;

      buffer[bytes_received] = '\0';

      string message_str = buffer;
    }
  }
}
