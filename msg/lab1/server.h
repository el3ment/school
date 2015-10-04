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

#include <string>
#include <vector>
#include <sstream>

#include <algorithm>
#include <fstream>
#include <map>

#include <stdlib.h>

using namespace std;

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

    struct Message{
    public:
        string name;
        string subject;
        string data;
    };

protected:
    virtual void create();
    virtual void close_socket();
    void serve();
    Command get_command(int);

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
    string cache_;
    map<string, vector<Message> > messages_;
};
