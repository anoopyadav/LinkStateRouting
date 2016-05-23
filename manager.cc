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
#include <time.h>
// Begin fork stuff
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>     /* General Utilities */
#include <string>
#include "manager.h"
#include "router.h"

using std::string;

// Get the IP address for the manager, Routers will need it
char addressBuffer[INET6_ADDRSTRLEN];
char port[6] = {'5', '4', '6', '8', '0', '\0'};

// Logger
void logMessage(string message, FILE* logFile)  {
	// Time
	time_t rawtime;
	struct tm * timeinfo;
	
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	
	char *timer = asctime(timeinfo);
	timer[strlen(timer) - 1] = '\0';
	
	fprintf(logFile, "[%s]:%s\n", timer, message.c_str());
}


// Detect the machines IP address
int getIpAddress()  {
	struct ifaddrs *ifAddrStruct;
    struct ifaddrs *ifa;
    void *tmpAddrPtr=NULL;
    //string ip = "0.0.0.0";

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            //char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //string interface = ifa->ifa_name;
            if(strcmp(ifa->ifa_name,"lo") != 0)  {
				//ipAddress = addressBuffer;
				//cout << "IP Address:" << addressBuffer << endl;
				//ip = addressBuffer;
				//printf("%s", addressBuffer);
				freeifaddrs(ifAddrStruct);
				return 0;
				//return ip;
			} 
        } else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //string interface = ifa->ifa_name;
            if(strcmp(ifa->ifa_name, "lo") != 0)  {
				//ipAddress = addressBuffer;
				//ip = addressBuffer;
				//printf("%s", addressBuffer);
				freeifaddrs(ifAddrStruct);
				return 0;
				//return ip;
			}
        } 
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return 1;
    //return ip;
}

int setupNetwork(string filename)  {
	// Routers ready messages
	int ready = 0;
	
	char message[100];
	
	getIpAddress();
    // Read the total number of routers
    FILE *fp;
    fp = fopen(filename.c_str(), "r");
   
    // This is the number of forked processes
    int routers;
    fscanf(fp, "%d", &routers);
	printf("Routers:%d\n", routers);
	//fprintf(logFile, "Total Routers on network:%d\n", routers);
   
    // Store the parent's pid
    pid_t parentPid = getpid();    
       
    /* now create new process */
    // Create the routers
    int pid;
    for(int i = 0; i < routers; i++)  {
		pid = fork();
        if(pid < 0)  {
			// Error
			exit(1);
		}
		if(pid == 0)  {
			// Router stuff
			sleep(5);
			//printf("I'm router number %d \n", getpid());
			routerSetup(addressBuffer, port);
			exit(0);
		}
    }
    
    // Manager code goes here
    FILE *logFile = fopen("manager.out", "w");
	if (logFile == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}
	
	memset(message, 0, sizeof message);
	sprintf(message, "Total routers as read from topology file: %d\n", routers);
	logMessage(message, logFile);
	sleep(2);
	
	
	
	// Read the topology in a 2D array
	int** topologyTable;
	int* t;

	topologyTable = (int**)malloc(routers * routers * sizeof(int*));
	t = (int*)malloc((routers * routers) * 3 * sizeof(int));
	for (int i = 0; i < routers * routers; i++) {
	  topologyTable[i] = t + (i * 3);
	  //printf("%d\n", i);
	}
	
	
	
	int first, second, third, topologyRows;
	topologyRows = 0;
	while(fscanf(fp, "%d", &first) != EOF)  {
		// End of topology table
		if(first == -1)  {
			break;
		}
		fscanf(fp, "%d%d", &second, & third);
		topologyTable[topologyRows][0] = first;
		topologyTable[topologyRows][1] = second;
		topologyTable[topologyRows][2] = third;
		//printf("%d, %d, %d\n", first, second, third);
		topologyRows++;
	}
	
    // Start the TCP Server
    struct addrinfo hints;
	struct addrinfo *servinfo, *p; // will point to the results
	int status;
	int sockfd, new_fd; // File Descriptors
	int yes;
	struct sockaddr_storage their_addr; // Store the client's information
	
	char s[INET6_ADDRSTRLEN];
	//printf("Welcome to Chat!\n");
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me
	
	// Call to addrinfo, get the connection information
	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		printf("getaddrinfo error:%s",gai_strerror(status));
		exit(1);
	}
	
	//cout << status;
	
	// Bind to the port
	for(p = servinfo; p != NULL; p = p->ai_next) {	
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			printf("server: socket\n");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			printf("setsockopt\n");
			exit(1);
		}
		if ((bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
			close(sockfd);
			printf("server: bind\n");
			continue;
		}
		break;
	}
	
	
	// Exit if bind fails
	if (p == NULL) {
		printf("server: failed to bind\n");
		return 2;
	}
	
	
	// free the linked-list
	freeaddrinfo(servinfo); 
	
	
	// Start listening
	if (listen(sockfd, 10) == -1) {
		printf("listen");
		exit(1);
	}
    
    // Store the file descriptors of all routers
    //int *fd;
	//fd = (int *) malloc(routers*sizeof(int));
	int** routersTable;
	int* temp;

	routersTable = (int**)malloc(routers * sizeof(int*));
	temp = (int*)malloc(routers * 2 * sizeof(int));
	for (int i = 0; i < routers; i++) {
	  routersTable[i] = temp + (i * 2);
	}
    
    // Select call
// Setup the select mechanism
    fd_set active_fd_set, read_fd_set;
    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);
    
    struct sockaddr_in clientname;
    socklen_t *sin_size = (socklen_t*)malloc(sizeof(socklen_t));
	*sin_size = sizeof their_addr;
	int count = 0;
    while (1)
         {
           /* Block until input arrives on one or more active sockets. */
           read_fd_set = active_fd_set;
           if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
             {
               //perror ("select");
               exit (1);
             }
             
           /* Service all the sockets with input pending. */
           for (int i = 0; i < FD_SETSIZE; ++i)  {
           //if(count == routers)
			//break;
             if (FD_ISSET (i, &read_fd_set))
               {
				   
				//printf("%d\n", i);
                 if (i == sockfd)
                   {
                     /* Connection request on original socket. */
                     int new1;
                     //size = sizeof (clientname);
                     new1 = accept (sockfd,
                                   (struct sockaddr *) &clientname,
                                   sin_size);
                     routersTable[count][0] = new1;
                     if (new1 < 0)
                       {
                         //perror ("accept");
                         exit (1);
                       }
             
                     FD_SET (new1, &active_fd_set);
                     
                     // Read
					int numbytes;
					char buffer[5];
					if ((numbytes = recv(new1, buffer, 4, 0)) == -1) {
						printf("\nError recieving message!\n");
						exit(1);
					}
		
					// Translate the int
					uint32_t translated_int, original_int;
					memcpy(&translated_int, buffer, sizeof translated_int);
					original_int = ntohl(translated_int);
					routersTable[count][1] = original_int;
					printf("Router is online on port:%d\n", original_int);
					//fprintf(logFile, "Router is online listening on UDP port: %d\n", original_int);
					memset(message, 0, sizeof message);
					sprintf(message, "Router is online listening on UDP port: %d\n", original_int);
					logMessage(message, logFile);
                    //printf("Router added!\n");
                    count++;
                    
                    // See if we've heard from all routers
                    if(count == routers)  {
						printf("\nAll routers have reported online. Sending neighbour lists...\n");
						memset(message, 0, sizeof message);
						sprintf(message, "All routers have reported online. Sending neighbour lists...\n");
						logMessage(message, logFile);
						
						memset(message, 0, sizeof message);
						sprintf(message, "\nRouter Table:");
						logMessage(message, logFile);
						for(int k = 0; k < routers; k++)  {
							fprintf(logFile, "%d %d\n", k, routersTable[k][1]);
						}
						fprintf(logFile, "\n");
						memset(message, 0, sizeof message);
						sprintf(message, "\nTopology Table:");
						logMessage(message, logFile);

						for(int k = 0; k < topologyRows; k++)  {
							fprintf(logFile, "%d %d %d\n", topologyTable[k][0], topologyTable[k][1], topologyTable[k][2]);
						}
						fprintf(logFile, "\n");
						
						// Send routers the association table here
						for(int o = 0; o < routers; o++)  {
							int sock = routersTable[o][0];
							memset(message, 0, sizeof message);
							sprintf(message, "Sending neighbour list to router %d on port %d\n", o, routersTable[o][1]);
							logMessage(message, logFile);
							
							// Write the router id number
							uint32_t translated_int = htonl(o);
							char *binary_string = (char *)&translated_int;
							if (send(sock, binary_string, 4, 0) == -1)  { 
								printf("send\n");
								close(sock);
								exit(0);
							}
							// Write the total number of routers in the network
							translated_int = htonl(routers);
							binary_string = (char*)&translated_int;
							if (send(sock, binary_string, 4, 0) == -1)  { 
								printf("send\n");
								close(sock);
								exit(0);
							}
							
							// Send the topology table
							for(int n = 0; n < topologyRows; n++)  {
								// Find enteries pertaining to the current router
								if(topologyTable[n][0] == o)  {
									translated_int = htonl(topologyTable[n][1]);
									binary_string = (char*)&translated_int;
									if (send(sock, binary_string, 4, 0) == -1)  { 
										printf("send\n");
										close(sock);
										exit(0);
									}
									translated_int = htonl(topologyTable[n][2]);
									binary_string = (char*)&translated_int;
									if (send(sock, binary_string, 4, 0) == -1)  { 
										printf("send\n");
										close(sock);
										exit(0);
									}
									int neighbourId = topologyTable[n][1];
									// send the udp port
									translated_int = htonl(routersTable[neighbourId][1]);
									//sleep(3);
									//translated_int = htonl(1000);
									binary_string = (char*)&translated_int;
									if (send(sock, binary_string, 4, 0) == -1)  { 
										printf("send\n");
										close(sock);
										exit(0);
									}
								}
								else if(topologyTable[n][1] == o)  {
									translated_int = htonl(topologyTable[n][0]);
									binary_string = (char*)&translated_int;
									if (send(sock, binary_string, 4, 0) == -1)  { 
										printf("send\n");
										close(sock);
										exit(0);
									}
									translated_int = htonl(topologyTable[n][2]);
									binary_string = (char*)&translated_int;
									if (send(sock, binary_string, 4, 0) == -1)  { 
										printf("send\n");
										close(sock);
										exit(0);
									}
									int neighbourId = topologyTable[n][0];
									// send the udp port
									translated_int = htonl(routersTable[neighbourId][1]);
									binary_string = (char*)&translated_int;
									if (send(sock, binary_string, 4, 0) == -1)  { 
										printf("send\n");
										close(sock);
										exit(0);
									}
								}
							}
							// Send list termination by sending a 1111
							translated_int = htonl(1111);
							binary_string = (char*)&translated_int;
							if (send(sock, binary_string, 4, 0) == -1)  { 
								printf("send");
								close(sock);
								exit(0);
							}
							
						}
						// Break out of original select, listen for repeats now
						break;
					}
					
                   }
                 else // message on already connected socket
                   {
                     int code = read_from_client(i);
                     // Ready Message = 6666
                     if(code == 6666)  {
						ready++;
						memset(message, 0, sizeof message);
						sprintf(message, "Ready message received from router.\n");
						logMessage(message, logFile);
						
						if(ready == routers)  {
							 printf("\nAll routers are ready to start communicating with each other!\n");
							 memset(message, 0, sizeof message);
							sprintf(message, "All routers are ready to start communicating with each other\n");
							logMessage(message, logFile);
							 // Send the routers OK message to connect to their neighbours
							 sleep(2);
							 printf("\nSending routers the go ahead to communicate with neighbours!\n");
							  memset(message, 0, sizeof message);
							sprintf(message, "Sending routers the go ahead to communicate with neighbours!\n");
							logMessage(message, logFile);
							 for(int p = 0; p < routers; p++)  {
								 memset(message, 0, sizeof message);
								sprintf(message, "Sending go ahead to communicate with neighbours to router %d on port %d!\n", p, routersTable[p][1]);
								logMessage(message, logFile);
								int sock = routersTable[p][0];
								
								// Give routers the go ahead = 7777
								uint32_t translated_int = htonl(7777);
								char *binary_string = (char *)&translated_int;
								if (send(sock, binary_string, 4, 0) == -1)  { 
									printf("send\n");
									close(sock);
									exit(0);
								}
							}
							ready = 0;
							break;
						}
					 }
					 
					 // Link ACKs done
					 if(code == 9999)  {
						ready++;
						memset(message, 0, sizeof message);
						sprintf(message, "Neighbour Link Setup Acknowledgment message received from router.\n");
						logMessage(message, logFile);  
						if(ready == routers)  {
							memset(message, 0, sizeof message);
							sprintf(message, "All routers are ready to exchange LSPs!\n");
							logMessage(message, logFile);
							printf("\nAll routers are ready to exchange LSPs!\n");
							memset(message, 0, sizeof message);
							sprintf(message, "Sending routers the go ahead to start exchanging LSPs.\n");
							logMessage(message, logFile);
							printf("\nSending routers the go ahead to start exchanging LSPs...\n");
							
							// Send routers goahead for exchanging LSPs = 8888
							for(int p = 0; p < routers; p++)  {
								int sock = routersTable[p][0];
								memset(message, 0, sizeof message);
								sprintf(message, "Sending go ahead to exchange LSPs to router %d on port %d\n", p, routersTable[p][1]);
								logMessage(message, logFile);
								
								// Give routers the go ahead = 8888
								uint32_t translated_int = htonl(8888);
								char *binary_string = (char *)&translated_int;
								if (send(sock, binary_string, 4, 0) == -1)  { 
									printf("send\n");
									close(sock);
									exit(0);
								}
							}
							ready = 0;
							break;
						}
					}
					
					// Wait for ready to accept message = 3333
					// Link ACKs done
					 if(code == 3333)  {
						ready++;
						memset(message, 0, sizeof message);
						sprintf(message, "All LSPs received message received from router.\n");
						logMessage(message, logFile);  
						if(ready == routers)  {
							memset(message, 0, sizeof message);
							sprintf(message, "All routers have received all LSPs!\n");
							logMessage(message, logFile);
							printf("\nAll routers have received all LSPs!\n");
							memset(message, 0, sizeof message);
							sprintf(message, "Sending data packet requests now...\n");
							logMessage(message, logFile);
							printf("\nSending data packet requests now...\n");
							
							int from, to;
							// Send data packet requests to routers
							while(fscanf(fp, "%d", &from) != EOF)  {
								// End of topology table
								if(from == -1)  {
									break;
								}
								fscanf(fp, "%d", &to);
								
								// Lookup the address of Router "from"
								uint32_t translated_int = htonl(to);
								char* binary_string = (char *)&translated_int;
								int sock = routersTable[from][0];
								if (send(sock, binary_string, 4, 0) == -1)  { 
									printf("send\n");
									close(sock);
									exit(0);
								}
								memset(message, 0, sizeof message);
								sprintf(message, "Sent data packet request to router %d\n", from);
								logMessage(message, logFile);
								//printf("Data message sent!\n");
								sleep(20);
							}
							
							printf("\nSending kill signal to all routers...\n");
							memset(message, 0, sizeof message);
							sprintf(message, "Sending kill signal to all routers...\n");
							logMessage(message, logFile);
							
							// Send kill message = 6789
							for(int p = 0; p < routers; p++)  {
								int sock = routersTable[p][0];
								
								// Give routers the kill signal = 6789
								uint32_t translated_int = htonl(6789);
								char *binary_string = (char *)&translated_int;
								if (send(sock, binary_string, 4, 0) == -1)  { 
									printf("send\n");
									close(sock);
									exit(0);
								}
								memset(message, 0, sizeof message);
								sprintf(message, "Sending kill signal to router %d on port %d...\n", p, routersTable[p][1]);
								logMessage(message, logFile);
							}
							break;
						}
					}
					 
                   }
               }
		   }
		   if(ready == routers)  {
			   //printf("Output router table\n", count);
			   //for(int k = 0; k < routers; k++)  {
				//   fprintf(logFile, "%d %d\n", k, routersTable[k][1]);
			   //}
				// Reset counter to count link ACKs from routers
				ready = 0;
			  break;
		   }
         }
    
    // Wait for routers to exit
    int wpid, stat;
    while ((wpid = wait(&stat)) > 0)
    {
        //printf("Router has exited!\n");
        sprintf(message, "Router has exited");
		logMessage(message, logFile);
    }
    
    memset(message, 0, sizeof message);
	sprintf(message, "All done. Exiting...");
	logMessage(message, logFile);
	printf("\n[Manager]: All done. Exiting...\n");
    
}

int read_from_client (int filedes)  {
	char buffer[4];
    int nbytes;
     
    nbytes = read (filedes, buffer, 4);
    if (nbytes < 0)  {
		/* Read error. */
        perror ("read");
        exit (EXIT_FAILURE);
    }
    else if (nbytes == 0)
        /* End-of-file. */
        return -1;
    else  {
        /* Data read. */
        uint32_t original_int1, translated_int1;
        memcpy(&translated_int1, buffer, sizeof translated_int1);
		original_int1 = ntohl(translated_int1);
		//printf("Ready signal:%d\n", original_int1);
        return original_int1;
    } 
}


int main(int argc, char** argv)  {
	if(argc != 2)  {
		printf("Usage: manager <topology_file>\n");
		return 1;
	}
	setupNetwork(argv[1]);
	return 0;
}


