#include "client.h"
#include "globals.h"

Client::Client() {
    // setup variables
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
}

Client::~Client() {
}

void Client::run() {
    
    cache_ = "";

    Command requestCommand;
    // connect to the server and run echo program
    create();
    while(requestCommand.command != "quit"){
        requestCommand = get_request();
        if(handle_request(requestCommand)){
            handle_response(requestCommand);
        }
    }
    close_socket();
}

void
Client::create() {
}

void
Client::close_socket() {
}

bool
Client::handle_request(Command requestCommand){
    if(requestCommand.command.empty()){
        cout << "Error: Command not found." << endl;
    
    }else if(requestCommand.command == "quit"){
    
    }else if(requestCommand.command == "reset"){
        send_request("reset\n");
        return true;

    }else if(requestCommand.command == "send"){
        if(requestCommand.params.size() >= 2){
            string subject;
            for(int i = 1; i < requestCommand.params.size(); i++)
                subject += requestCommand.params[i];

            send_request("put " 
                + requestCommand.params[0] + " "
                + subject + " "
                + to_string(requestCommand.data.length()) 
                + "\n" + requestCommand.data);
            
            return true;

        }else{
            cout << "Error: Not enough parameters";
        }

    }else if(requestCommand.command == "list"){
        if(requestCommand.params.size() == 1){
            send_request("list " + requestCommand.params[0] + "\n");
            
            return true;

        }else{
            cout << "Error: Invalid parameters" << endl;
        }
    
    }else if(requestCommand.command == "read"){
        if(requestCommand.params.size() == 2){
            send_request("get " 
                + requestCommand.params[0] + " "
                + requestCommand.params[1] + "\n");
            
            return true;

        }else{
            cout << "Error: Invalid parameters" << endl;
        }
        
    }else{
        cout << "Error: Command not found." << endl;
    }

    return false;
}

void
Client::handle_response(Command requestCommand){
    
    string response = get_line();

    if(requestCommand.command == "send" or requestCommand.command == "reset"){
        if(response != "OK\n"){
            cout << "Unexpected server response: " << response;
        }
    }else if(requestCommand.command == "list"){
        if(response.substr(0,4) != "list"){
            cout << "Unexpected server response: " << response;
        }
    
        vector<string> command = split_line(response);
        cout << get_n_lines(atoi(command[1].c_str()));

    }else if(requestCommand.command == "read"){
        if(response.substr(0,7) != "message"){
            cout << "Unexpected server response: " << response;
        }

        vector<string> command = split_line(response);
        string message = get_n_chars(atoi(command[command.size() - 1].c_str()));
        
        if(message == ""){
            cout << "Server did not send entire message" << endl;
        }else{
            for(int i = 1; i < command.size() - 1; i++)
                cout << command[i];
            cout << endl << message;
        }

    }else{
        cout << "Error: Command not found." << endl;
    }
}

Client::Command
Client::parse_command(string line){
    Command parsedCommand;

    parsedCommand.params = vector<string>();

    istringstream iss(line);
    string temp;
    while (getline( iss, temp, ' ')) {
        if(parsedCommand.command.empty())
            parsedCommand.command = temp;
        else 
            parsedCommand.params.push_back(temp);
    }

    return parsedCommand;

}


// TODO: fold split_line into parse_command
vector<string> 
Client::split_line(string line){
    vector<string> split = vector<string>();

    istringstream iss(line);
    string temp;
    while (getline( iss, temp, ' ')) {
        split.push_back(temp);
    }

    return split;
}

string
Client::get_data(){
    string line;
    string data;

    while (true){
        while(getline(cin,line)){
            if(line == ""){
                return data;
            }
            data += line + "\n";
        }
    }
}

Client::Command
Client::get_request() {
    string line;
    Command requestCommand;

    cout << "% ";
    
    // loop to handle user interface
    getline(cin,line);

    requestCommand = parse_command(line);

    if(requestCommand.command == "send"){
        cout << "- Type your message. End with a blank line -" << endl;
        requestCommand.data = get_data();
    }

    return requestCommand;
}

bool
Client::send_request(string request) {
    // prepare to send request
    const char* ptr = request.c_str();
    int nleft = request.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(server_, ptr, nleft, 0)) < 0) {
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

string
Client::get_n_lines(int n){
    string response = "";

    for(int i = 0; i < n; i++){
        response += get_line();
    }

    return response;
}

string
Client::get_line(){

    string response = get_response();

    if(response.length() > 0){
        cache_ = response.substr(response.find_first_of("\n") + 1, 
                    response.length() - response.find_first_of("\n"));
        response = response.substr(0, response.find_first_of("\n")) + "\n";
    }

    return response;

}

string
Client::get_n_chars(int n){

    string response = get_response(n);

    if(response.length() > 0){
        cache_ = response.substr(n, response.length() - n);
        response = response.substr(0, n);
    }

    return response;
}

string
Client::get_response(int length) {
    string response = cache_;
    // read until we get a newline
    while ((length == -1 and response.find("\n") == string::npos)
        or (length > 0 and response.length() < length)){
        
        int nread = recv(server_,buf_,1024,0);
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
        response.append(buf_,nread);

    }
    
    return response;
}
