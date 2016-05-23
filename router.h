#ifndef ROUTER_H
#define ROUTER_H
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
#include <vector>
#include <string>
#include <vector>
#include <iostream>
#include <istream>
#include <ostream>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <cstdlib>
int routerSetup(char* managerIp, char* managerPort);
std::string packMessage(int** topologyTable, int topologyRows, int routerId);
#endif
