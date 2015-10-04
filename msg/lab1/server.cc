#include "server.h"

Server::Server() {
    // setup variables
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
}

Server::~Server() {
    delete buf_;
}

void
Server::run() {
    // create and run the server
    create();
    serve();
}

void
Server::create() {
}

void
Server::close_socket() {
}

void
Server::serve() {
    // setup client
    int client;
    struct sockaddr_in client_addr;
    socklen_t clientlen = sizeof(client_addr);

      // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {
        cache_ = "";
        Command requestCommand;
        do{

            requestCommand = get_command(client);
            if(requestCommand.command != ""){
                if(handle_command(client, requestCommand)){
                    handle_success(client, requestCommand);
                }else{
                    send_response(client, "error there was an error handling the command\n");
                }
            }

        } while(requestCommand.command != "");

        close(client);
    }
    close_socket();
}

Server::Command
Server::parse_header(string request) {
    Command responseMessage;

    responseMessage.params = vector<string>();

    istringstream iss(request);
    string temp;
    while (getline( iss, temp, ' ')) {
        if(responseMessage.command.empty())
            responseMessage.command = temp;
        else 
            responseMessage.params.push_back(temp);
    }

    return responseMessage;
}

void
Server::handle_success(int client, Command command){
    if(command.command == "put"
    or command.command == "reset"){
        send_response(client, "OK\n");    
    }else if(command.command == "get"){
        string name = command.params[0];
        int index = atoi(command.params[1].c_str());
        Message message = messages_[name][index - 1];

        send_response(client, "message " 
            + message.subject + " " 
            + to_string(message.data.length()) + "\n" + message.data);

    }else if(command.command == "list"){
        string response = "list " + to_string(messages_[command.params[0]].size()) + "\n";
        for(int i = 0; i < messages_[command.params[0]].size(); i++){
            Message message = messages_[command.params[0]][i];
            response += to_string(i+1) + " " + message.subject + "\n";
        }

        send_response(client, response);
    }
}

Server::Command
Server::get_command(int client) {
    Command message;

    // get a request
    string command = get_line(client, false);

    // break if client is done or an error occurred
    if (command.empty())
        return message;

    // parse request
    message = parse_header(command);

    if(message.command == "put"){
        int chars = atoi(message.params[message.params.size() - 1].c_str());
        message.value = get_n_chars(client, chars);
    }

    return message;
    
}

bool
Server::handle_command(int client, Command command){
    if(command.command == "put" and command.params.size() >= 3){
        Message message;
        message.name = command.params[0];
        for(int i = 1; i < command.params.size() - 1; i++){
            message.subject += command.params[i];
        }
        message.data = command.value;

        //if(messages_.find(message.name) == messages_.end())
        //    messages_.insert(message.name, vector<Message>());
        messages_[message.name].push_back(message);

        return true;
    }else if(command.command == "reset"){
        messages_.clear();
        return true;
    }else if(command.command == "list"){
        return command.params.size() == 1;
    }else if(command.command == "get"){
        return command.params.size() == 2
            and messages_[command.params[0]].size() >= atoi(command.params[1].c_str());
    }

    // Error!
    return false;
}

string
Server::get_n_lines(int client, int n){
    string request = "";

    for(int i = 0; i < n; i++){
        request += get_line(client);
    }

    return request;
}

string
Server::get_line(int client, bool includeNewline){

    string request = get_request(client);

    if(request.length() > 0){
        cache_ = request.substr(request.find_first_of("\n") + 1, 
                    request.length() - request.find_first_of("\n"));
        request = request.substr(0, request.find_first_of("\n")) + (includeNewline ? "\n" : "");
    }

    return request;

}

string
Server::get_n_chars(int client, int n){

    string request = get_request(client, n);

    if(request.length() > 0){
        cache_ = request.substr(n, request.length() - n);
        request = request.substr(0, n);
    }

    return request;
}

string
Server::get_request(int client, int length) {
    string request = cache_;
    // read until we get a newline
    while ((length == -1 and request.find("\n") == string::npos)
        or (length > 0 and request.length() < length)){

        int nread = recv(client,buf_,1024,0);
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } else if (nread == 0) {
            // the socket is closed
            return "";
        }
        // be sure to use append in case we have binary data
        request.append(buf_,nread);

    }
    
    return request;
}


bool
Server::send_response(int client, string response) {

    // prepare to send response
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;

    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
            if (errno == EINTR) {
                // the socket call was interrupted -- try again
                continue;
            } else {
                // an error occurred, so break out
                perror("write");
                return false;
            }
        } else if (nwritten == 0) {
            // the socket is closed
            return false;
        }
        nleft -= nwritten;
        ptr += nwritten;

    }

    return true;
}
