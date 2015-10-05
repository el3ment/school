#include "server.h"

Server::Server() {
    // setup variables
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
    messages_ = MessageMap();

    // serverLock = sem_open("lockServerBlah", O_CREAT, 0600, 1);
    // cacheLock = sem_open("cacheLockBlah", O_CREAT, 0600, 1);
    // printLock = sem_open("printLockBlah", O_CREAT, 0600, 1);
    // numClientsWaiting = sem_open("numClientsWaitingBlah", O_CREAT, 0600, 0);
    // maxClientSpaces = sem_open("maxClientSpacesBlah", O_CREAT, 0600, 1024);

    sem_init(serverLock, 0600, 1);
    sem_init(cacheLock, 0600, 1);
    sem_init(printLock, 0600, 1);
    sem_init(numClientsWaiting, 0600, 0);
    sem_init(maxClientSpaces, 0600, 1024);

}

void *
worker(void *vptr){
    Server* server = (Server*) vptr;
    int client;

    // If the get_client() function returns a -1, it means it's time to kill
    // the thread
    while((client = server->get_client()) > -1){
        server->debug("got client", client);
        server->debug("waiting on command", client);
        Server::Command requestCommand = server->get_command(client);
        server->debug("got command" + requestCommand.command, client);
        if(requestCommand.command != ""){

            if(server->handle_command(client, requestCommand)){
                server->debug("sending response", client);
                server->handle_success(client, requestCommand);
                server->debug("sent response", client);
            }else{
                server->debug("sending error", client);
                server->send_response(client, "error there was an error handling the command\n");
                server->debug("sent error", client);
            }

            // give him back!
            server->push_client(client);
            
        }else{
            server->debug("closing client", client);
            // remove client from queue
            close(client);
        }

        server->debug("waiting on client");
        
    }

    server->debug("killing worker");

    return 0;
}

void 
Server::debug(string str){
    debug(str, -1);
}

void
Server::debug(string str, int client){
    // sem_wait(printLock);
    // cout << messages_.number_of_messages("user1") << " : " << client << " : " << str << endl;
    // sem_post(printLock);
}

Server::~Server() {
    delete buf_;
}

void
Server::setCache(int client, string content){
    //sem_wait(cacheLock);

    cache_[client] = content;

    //sem_post(cacheLock);
}

string
Server::getCache(int client){
    //sem_wait(cacheLock);

    string resp = cache_[client];

    //sem_post(cacheLock);

    return resp;
}


void
Server::run() {
    // create and run the server
    create();
    pool();
    serve();
}

void Server::create() { }
void Server::close_socket() { }

void
Server::pool() {
    pthread_t pool_[10];
    for(int i = 0; i < 10; i++){
        pthread_create(&pool_[i], NULL, &worker, this);
    }
}


void
Server::serve() {
    // setup client
    int client;
    struct sockaddr_in client_addr;
    socklen_t clientlen = sizeof(client_addr);
    status_ = 1;

    debug("Accepting clients");

      // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {
        setCache(client, "");
        push_client(client);
    }

    status_ = -1;
    close_socket();
}



int
Server::get_client(){
    debug("get_client waiting on clientsWaiting");
    sem_wait(numClientsWaiting);
    debug("get_client got clientsWaiting");
    debug("get_client waiting on serverLock");
    sem_wait(serverLock);
    debug("get_client got serverlock");

    int client = -1;

    if(status_ == 1){
        client = clients_.front();
        clients_.pop();
    }

    sem_post(serverLock);
    sem_post(maxClientSpaces);

    return client;
}

void
Server::push_client(int client){
    debug("push_client waiting on clientsSpaces");
    sem_wait(maxClientSpaces);
    debug("push_client got clientsSpaces");
    debug("push_client waiting on serverLock");
    sem_wait(serverLock);
    debug("push_client got serverLock");
    
    clients_.push(client);

    sem_post(serverLock);
    sem_post(numClientsWaiting);
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

        string response = "message " 
            + message.subject + " " 
            + to_string(message.data.length()) + "\n" + message.data;

        send_response(client, response);

    }else if(command.command == "list"){
        vector<Message> messages = messages_.get(command.params[0]);
        
        string response = "list " + to_string(messages.size()) + "\n";
        for(int i = 0; i < messages.size(); i++){
            response += to_string(i+1) + " " + messages[i].subject + "\n";
        }

        send_response(client, response);
    }
}

Server::Command
Server::get_command(int client) {
    Command message;

    debug("getting line", client);
    // get a request
    string command = get_line(client, false);

    debug("got line", client);

    // break if client is done or an error occurred
    if (command.empty())
        return message;

    // parse request
    message = parse_header(command);

    debug("getting data", client);

    if(message.command == "put"){
        int chars = atoi(message.params[message.params.size() - 1].c_str());
        message.value = get_n_chars(client, chars);
    }

    debug("got data", client);

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
        setCache(client, request.substr(request.find_first_of("\n") + 1, 
                    request.length() - request.find_first_of("\n")));
        request = request.substr(0, request.find_first_of("\n")) + (includeNewline ? "\n" : "");
    }

    return request;

}

string
Server::get_n_chars(int client, int n){

    string request = get_request(client, n);

    if(request.length() > 0){
        setCache(client, request.substr(n, request.length() - n));
        request = request.substr(0, n);
    }

    return request;
}

string
Server::get_request(int client, int length) {
    string request = getCache(client);
    char* buffer = new char[buflen_+1];
    debug("getting request - the cache so far is : " + request, client);

    // It's possible to ask for 0 chars with an empty cache,
    // and it will look like a closed socket

    // read until we get a newline
    while ((length == -1 and request.find("\n") == string::npos)
        or (length > 0 and request.length() < length)){

        debug("recv from socket", client);
        
        int nread = recv(client,buffer,1024,0);

        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                debug("socket error:" + to_string(errno), client);

                return "";
        } else if (nread == 0) {
            // the socket is closed
            debug("socket closed", client);
            return "";
        }
        // be sure to use append in case we have binary data
        request.append(buffer,nread);

    }

    delete buffer;
    
    return request;
}


bool
Server::send_response(int client, string response) {

    // prepare to send response
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;

    debug("sending response - " + response.substr(0, 10), client);

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
