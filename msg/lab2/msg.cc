#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include "inet-client.h"
#include "globals.h"

using namespace std;

bool PRINT_DEBUG = false;

int
main(int argc, char **argv)
{
    int option;

    // setup default arguments
    int port = 5000;
    string host = "localhost";

    // process command line options using getopt()
    // see "man 3 getopt"
    while ((option = getopt(argc,argv,"s:p:d:")) != -1) {
        switch (option) {
            case 's':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                PRINT_DEBUG = true;
                break;
                
            default:
                cout << "Usage:" << endl;
                cout << "client [-s host] [-p port] [-d]" << endl << endl;

                cout << "Argument           Definition" << endl;
                cout << "---------------------------------------------------------" << endl;
                cout << "-s [server]        machine name of the messaging server (e.g. hiking.cs.byu.edu)" << endl;
                cout << "-p [port]          port number of the messaging server" << endl;
                cout << "-d                 print debugging information" << endl;

                exit(EXIT_FAILURE);
        }
    }

    InetClient client = InetClient(host, port);
    client.run();
}

