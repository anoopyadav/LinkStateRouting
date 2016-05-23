#ifndef MANAGER_H
#define MANAGER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <string>
void logMessage(std::string message, FILE* logfile);
int getIpAddress();
int setupNetwork(std::string filename);
int read_from_client(int filedes);
#endif
