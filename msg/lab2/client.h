#pragma once

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <stdlib.h>

using namespace std;

class Client {
public:
    Client();
    ~Client();

    void run();

    struct Command {
        public:
            string command;
            vector<string> params;
            string data;
    };

protected:
    virtual void create();
    virtual void close_socket();
    Command get_request();
    Command parse_command(string);
    bool handle_request(Command);
    void handle_response(Command);
    string get_data();
    bool send_request(string);
    string get_response(int = -1);
    string get_n_chars(int);
    string get_line();
    string get_n_lines(int);
    vector<string> split_line(string);

    int server_;
    int buflen_;
    char* buf_;
    string cache_;
};
