#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include "inet-server.h"
#include "globals.h"

using namespace std;

bool PRINT_DEBUG = false;

int
main(int argc, char **argv)
{
    int option, port;

    // setup default arguments
    port = 5000;

    // process command line options using getopt()
    // see "man 3 getopt"
    while ((option = getopt(argc,argv,"p:d:")) != -1) {
        switch (option) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                PRINT_DEBUG = true;
            default:
                cout << "server [-p port] [-d]" << endl << endl;

                cout << "Argument           Definition" << endl;
                cout << "---------------------------------------------------------" << endl;
                cout << "-p [port]          port number of the messaging server" << endl;
                cout << "-d                 print debugging information" << endl;

                exit(EXIT_FAILURE);
        }
    }

    InetServer server = InetServer(port);
    server.run();
}
