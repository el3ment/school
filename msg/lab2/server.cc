#include "server.h"

Server::Server() {
    // setup variables
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
    messages_ = MessageMap();

    sem_init(&lock, 0600, 1);
    sem_init(&numClientsWaiting, 0, 0);
    sem_init(&maxClientSpaces, 0, 1024);
}

void *
worker(void *vptr){
    Server* server = (Server*) vptr;
    int client;

    // If the get_client() function returns a -1, it means it's time to kill
    // the thread
    while((client = server->get_client()) > -1){
        Server::Command requestCommand = server->get_command(client);
        if(requestCommand.command != ""){
            if(server->handle_command(client, requestCommand)){
                server->handle_success(client, requestCommand);
            }else{
                server->send_response(client, "error there was an error handling the command\n");
            }

            // give him back!
            server->push_client(client);
        }else{
            // remove client from queue
            server->cache_[client] = ""; // not sure if this is nessesary, since we clear it on accept
            close(client);
        }
    }

    return 0;
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
    pthread_t pool_[10];
    for(int i = 0; i < 10; i++){
        pthread_create(&pool_[i], NULL, &worker, this);
        pthread_join(pool_[i], NULL);
    }
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
    status_ = 1;

      // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {
        push_client(client);
    }

    status_ = -1;
    close_socket();
}

int
Server::get_client(){
    sem_wait(&numClientsWaiting);
    sem_wait(&lock);
    int client = -1;

    if(status_ == 1){
        client = clients_.front();
        clients_.pop();
    }

    sem_post(&lock);
    sem_post(&maxClientSpaces);

    return client;
}

void
Server::push_client(int client){    
    sem_wait(&maxClientSpaces);
    sem_wait(&lock);
    
    clients_.push(client);

    sem_post(&lock);
    sem_post(&numClientsWaiting);
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
        int index = atoi(command.params[1].c_str()) - 1;
        Message message = messages_.get(name, index); //[name][index - 1];

        send_response(client, "message " 
            + message.subject + " " 
            + to_string(message.data.length()) + "\n" + message.data);

    }else if(command.command == "list"){
        string response = "list " + to_string(messages_.number_of_messages(command.params[0])) + "\n";
        for(int i = 0; i < messages_.number_of_messages(command.params[0]); i++){
            Message message = messages_.get(command.params[0], i);
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

        messages_.put(message.name, message);

        return true;
    }else if(command.command == "reset"){
        messages_.clear();
        return true;
    }else if(command.command == "list"){
        return command.params.size() == 1;
    }else if(command.command == "get"){
        return command.params.size() == 2
            and messages_.test(command.params[0], atoi(command.params[1].c_str()) - 1);
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
        cache_[client] = request.substr(request.find_first_of("\n") + 1, 
                    request.length() - request.find_first_of("\n"));
        request = request.substr(0, request.find_first_of("\n")) + (includeNewline ? "\n" : "");
    }

    return request;

}

string
Server::get_n_chars(int client, int n){

    string request = get_request(client, n);

    if(request.length() > 0){
        cache_[client] = request.substr(n, request.length() - n);
        request = request.substr(0, n);
    }

    return request;
}

string
Server::get_request(int client, int length) {
    string request = cache_[client];
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
