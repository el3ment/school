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
    sem_t lock;
    map<string, vector<Message> > messages_;

public:
    
    MessageMap(){
        sem_init(&lock, 0, 1);
    }
    
    ~MessageMap(){

    }

    int number_of_messages(string username){
        sem_wait(&lock);
        
        int size = messages_[username].size();
        
        sem_post(&lock);
        
        return size;
    }

    Message get(string username, int index){
        sem_wait(&lock);

        Message msg = messages_[username][index];

        sem_post(&lock);

        return msg;
    }

    void clear(){
        sem_wait(&lock);
        
        messages_.clear();

        sem_post(&lock);
    }

    bool test(string username, int index){
        sem_wait(&lock);

        bool test = index < messages_[username].size() and index >= 0;

        sem_post(&lock);

        return test;
    }
    
    void put(string username, Message message){
        sem_wait(&lock);

        messages_[username].push_back(message);

        sem_post(&lock);
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

    virtual void create();
    virtual void close_socket();
    void serve();

    Command get_command(int);
    int get_client();
    void push_client(int);

    string get_request(int, int = -1);
    string get_n_chars(int, int);
    string get_line(int, bool = true);
    string get_n_lines(int, int);
    Command parse_header(string);

    bool send_response(int, string);
    bool handle_command(int, Command);
    void handle_success(int, Command);

    int server_;
    int buflen_;
    char* buf_;
    map<int, string> cache_;
    MessageMap messages_;
    pthread_t pool_[10];
    queue<int> clients_;
    int status_;

    sem_t lock;
    sem_t maxClientSpaces;
    sem_t numClientsWaiting;


};
    
