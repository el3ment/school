#pragma once

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>

#include <string>
#include <vector>
#include <queue>
#include <sstream>

#include <algorithm>
#include <fstream>
#include <map>

#include <stdlib.h>

using namespace std;

struct Message{
    public:
        string name;
        string subject;
        string data;
    };

class MessageMap {
private:
    sem_t messageMapLock;
    map<string, vector<Message> > messages_;

public:
    
    MessageMap(){
        //messageMapLock = sem_open("messageMapLockNew", O_CREAT, 0600, 1);
        sem_init(*messageMapLock, 0600, 1);
    }
    
    ~MessageMap(){

    }

    int number_of_messages(string username){
        sem_wait(*messageMapLock);
        
        int size = messages_[username].size();
        
        sem_post(*messageMapLock);
        
        return size;
    }

    vector<Message> get(string username){
        sem_wait(*messageMapLock);

        vector<Message> msgs = messages_[username];

        sem_post(*messageMapLock);

        return msgs;
    }

    Message get(string username, int index){
        sem_wait(*messageMapLock);

        Message msg = messages_[username][index];

        sem_post(*messageMapLock);

        return msg;
    }

    void clear(){
        sem_wait(*messageMapLock);

        messages_.clear();

        sem_post(*messageMapLock);
    }

    bool test(string username, int index){
        sem_wait(*messageMapLock);

        bool test = index < messages_[username].size() and index >= 0;

        sem_post(*messageMapLock);

        return test;
    }
    
    void put(string username, Message message){
        sem_wait(*messageMapLock);

        messages_[username].push_back(message);

        sem_post(*messageMapLock);
    }

};

class Server {
public:
    Server();
    ~Server();

    void run();

    struct Command {
    public:
        string command;
        vector<string> params;
        string value;
        int remaining;
    };

    struct Worker {
    public:
        Server* server;
        int id;
    };

    virtual void create();
    virtual void close_socket();
    void serve();

    Command get_command(int);
    int get_client();
    void push_client(int);
    void pool();

    string get_request(int, int = -1);
    string get_n_chars(int, int);
    string get_line(int, bool = true);
    string get_n_lines(int, int);
    Command parse_header(string);

    bool send_response(int, string);
    bool handle_command(int, Command);
    void handle_success(int, Command);

    void setCache(int client, string);
    string getCache(int client);

    void debug(string);
    void debug(string, int);

    int server_;
    int buflen_;
    char* buf_;
    map<int, string> cache_;
    MessageMap messages_;
    pthread_t pool_[10];
    queue<int> clients_;
    int status_;

    sem_t serverLock;
    sem_t maxClientSpaces;
    sem_t numClientsWaiting;
    sem_t cacheLock;
    sem_t printLock;


};
    
