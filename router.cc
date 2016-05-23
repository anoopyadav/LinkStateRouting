#include "router.h"
#include "manager.h"
#include "dijkstra.h"

using std::string;
using std::vector;
using std::istringstream;
using std::ostringstream;
using std::to_string;
using std::istream_iterator;
using std::back_inserter;
using std::stringstream;
using std::list;
using std::ostream_iterator;
using std::cout;
using std::endl;

string packMessage(int** topologyTable, int topologyRows, int routerId)  {
	ostringstream oss;
	oss << 5678 << " " << routerId << " " << topologyRows << " ";
	for(int i = 0; i < topologyRows; i++)  {
		oss << topologyTable[i][0] << " " <<	topologyTable[i][1] << " " << topologyTable[i][2] << " ";
	}
	return oss.str();

}
int routerSetup(char* managerIp, char* managerPort)  {
	char message[100];
	// Router Specific variables
	int routerId;
	int routers;
	int** topologyTable;
	int* t;
	int topologyRows = 0;
	int neighbours = 0;

	topologyTable = (int**)malloc(10 * 10 * sizeof(int*));
	t = (int*)malloc((10 * 10) * 3 * sizeof(int));
	for (int i = 0; i < 10 * 10; i++) {
	  topologyTable[i] = t + (i * 3);
	  //printf("%d\n", i);
	}
	// End state variables
	
	int sockfd, numbytes, newfd;
	//char buffer[BUFFER_SIZE];
	struct addrinfo hints, *serverInfo, *p;
	int rv;
	char ipAddress[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; //always the same. At least in this class
	hints.ai_socktype = SOCK_STREAM; //set tcp


	//get information about the host and port specified by the user
	if ((rv = getaddrinfo(managerIp, managerPort, &hints, &serverInfo)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = serverInfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	
	// Start UDP server
	int sockfd1;
	struct addrinfo hints1, *servinfo1, *p1;
	int rv1;
	int numbytes1;
	struct sockaddr_storage their_addr1;
	//char buf[MAXBUFLEN];
	socklen_t addr_len1;
	//char s[INET6_ADDRSTRLEN];
	memset(&hints1, 0, sizeof hints1);
	hints1.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints1.ai_socktype = SOCK_DGRAM;
	hints1.ai_flags = AI_PASSIVE; // use my IP
	if ((rv1 = getaddrinfo(NULL, "0", &hints1, &servinfo1)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv1));
		return 1;
	}
	// loop through all the results and bind to the first we can
	for(p1 = servinfo1; p1 != NULL; p1 = p1->ai_next) {
		if ((sockfd1 = socket(p1->ai_family, p1->ai_socktype, p1->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd1, p1->ai_addr, p1->ai_addrlen) == -1) {
			close(sockfd1);
			perror("listener: bind");
			continue;
		}
		break;
	}
	if (p1 == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}
	struct sockaddr_in sin;
	socklen_t addrlen1 = sizeof(sin);
	int udpPort;
	if(getsockname(sockfd1, (struct sockaddr *)&sin, &addrlen1) == 0 &&
	   sin.sin_family == AF_INET &&
	   addrlen1 == sizeof(sin))
	{
		 udpPort = ntohs(sin.sin_port);
	}
	
	freeaddrinfo(servinfo1);
	addr_len1 = sizeof their_addr1;
	//printf("listener: waiting to recvfrom...\n");
	// End UDP listener setup
	
	// Send the UDP port of the node to the manager
	freeaddrinfo(serverInfo); // all done with this structure
	uint32_t translated_int = htonl(udpPort);
	char *binary_string = (char *)&translated_int;
	if (send(sockfd, binary_string, 4, 0) == -1)  { 
					printf("send\n");
					close(sockfd);
					exit(0);
	}
	// Recieve the node number assigned to the router
	char buffer[4];
	if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
		printf("\nError recieving message!\n");
		exit(1);
	}
	uint32_t translated_int1, original_int1;
	memcpy(&translated_int1, buffer, sizeof translated_int1);
	original_int1 = ntohl(translated_int1);
	routerId = original_int1;
	//printf("Router Id is:%d\n", routerId);
	  

	// Recieve the total number of routers
	if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
		printf("\nError recieving message!\n");
		exit(1);
	}
	memcpy(&translated_int1, buffer, sizeof translated_int1);
	original_int1 = ntohl(translated_int1);
	routers = original_int1;
	//printf("Total routers are %d\n", routers);
	// Recieve the topology table
	//sleep(10);

	// Recieve the topology table for the node
	while(1)  {
		if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
			printf("\nError recieving message!\n");
			exit(1);
		}
		memcpy(&translated_int1, buffer, sizeof translated_int1);
		original_int1 = ntohl(translated_int1);
		
		
		// End of topology table
		if(original_int1 == 1111)  {
			break;
		}
		neighbours++;
		
		// Neighbour id
		topologyTable[topologyRows][0] = original_int1;
		//printf("Neighbourid:%d\n", original_int1);
		
		if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
			printf("\nError recieving message!\n");
			exit(1);
		}
		memcpy(&translated_int1, buffer, sizeof translated_int1);
		original_int1 = ntohl(translated_int1);
		
		// Link Weight
		topologyTable[topologyRows][1] = original_int1;
		//printf("Link Weight:%d\n", original_int1);
		
		if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
			printf("\nError recieving message!\n");
			exit(1);
		}
		memcpy(&translated_int1, buffer, sizeof translated_int1);
		original_int1 = ntohl(translated_int1);
		//printf("UDP port:%d\n", original_int1);
		
		// UDP port
		topologyTable[topologyRows][2] = original_int1;
		topologyRows++;
	}
	
	// Send ready message to manager
	translated_int = htonl(6666);
	binary_string = (char *)&translated_int;
	if (send(sockfd, binary_string, 4, 0) == -1)  { 
		printf("send\n");
		close(sockfd);
		exit(0);
	}
	
	// Write the topology table
	char name[13] = {'r', 'o', 'u', 't', 'e', 'r'};
	sprintf(name + 6, "%d.out", routerId);
	FILE *logFile = fopen(name, "w");
	if (logFile == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}
	memset(message, 0, sizeof message);
	sprintf(message, "Router ID is %d.\n", routerId);
	logMessage(message, logFile);
	
	memset(message, 0, sizeof message);
	sprintf(message, "\nTopology Table:");
	logMessage(message, logFile);
	for(int i = 0; i < topologyRows; i++)  {
		fprintf(logFile, "%d %d %d\n", topologyTable[i][0], topologyTable[i][1], topologyTable[i][2]);
	}
	fprintf(logFile, "\n");
	//sleep(20);
	
	// Wait for the go-ahead
	if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
		printf("\nError recieving message!\n");
		exit(1);
	}
	memcpy(&translated_int1, buffer, sizeof translated_int1);
	original_int1 = ntohl(translated_int1);
	//printf("Go ahead Received!\n", original_int1);
	
	memset(message, 0, sizeof message);
	sprintf(message, "Go ahead received from manager!\n");
	logMessage(message, logFile);
	//fprintf(logFile, "Go ahead received!\n");
	
	//fprintf(logFile, "Neighbours: %d\n", neighbours);
	
	sleep(3);
	// Send link establishment message = nodeid
	for(int i = 0; i < neighbours; i++)  {
		int sock;
		struct addrinfo hints, *servinfo, *p;
		int rv;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		string s = to_string(topologyTable[i][2]);
		char const *pchar = s.c_str();  //use char const* as target type
		if ((rv = getaddrinfo(managerIp, pchar, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}
		// loop through all the results and make a socket
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sock = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
				perror("talker: socket");
				continue;
			}
			break;
		}
		if (p == NULL) {
			fprintf(stderr, "talker: failed to bind socket\n");
			return 2;
		}
		
		translated_int = htonl(routerId);
		binary_string = (char *)&translated_int;
		if ((numbytes = sendto(sock, binary_string, 4, 0,
			p->ai_addr, p->ai_addrlen)) == -1) {
			perror("talker: sendto");
			exit(1);
		}
		freeaddrinfo(servinfo);
		memset(message, 0, sizeof message);
		sprintf(message, "Sent initialize link message to node %d\n", topologyTable[i][0]);
		logMessage(message, logFile);
		//fprintf(logFile, "Sent initialize link message to node %d\n", topologyTable[i][0]);
		
	}
	
	sleep(5);
	// Nodes recieve Init requests
	int count = 0;
	while(count < neighbours)  {
		//printf("Here!\n");
		if ((numbytes = recvfrom(sockfd1, buffer, 4 , 0,
			(struct sockaddr *)&their_addr1, &addr_len1)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		memcpy(&translated_int1, buffer, sizeof translated_int1);
		original_int1 = ntohl(translated_int1);


		//printf("Recieved init request! %d\n", original_int1);
		memset(message, 0, sizeof message);
		sprintf(message, "Received initialize link request from router %d\n", original_int1);
		logMessage(message, logFile);
		//fprintf(logFile, "Received initialize link request from router %d\n", original_int1);
		count++;
	}
	
	sleep(5);
	// Send Acks
	for(int i = 0; i < neighbours; i++)  {
		int sock;
		struct addrinfo hints, *servinfo, *p;
		int rv;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		string s = to_string(topologyTable[i][2]);
		char const *pchar = s.c_str();  //use char const* as target type
		if ((rv = getaddrinfo(managerIp, pchar, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}
		// loop through all the results and make a socket
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sock = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
				perror("talker: socket");
				continue;
			}
			break;
		}
		if (p == NULL) {
			fprintf(stderr, "talker: failed to bind socket\n");
			return 2;
		}
		
		translated_int = htonl(routerId);
		binary_string = (char *)&translated_int;
		if ((numbytes = sendto(sock, binary_string, 4, 0,
			p->ai_addr, p->ai_addrlen)) == -1) {
			perror("talker: sendto");
			exit(1);
		}
		freeaddrinfo(servinfo);
		memset(message, 0, sizeof message);
		sprintf(message, "Sent ACK link message to node %d\n", topologyTable[i][0]);
		logMessage(message, logFile);
		//fprintf(logFile, "Sent ACK link message to node %d\n", topologyTable[i][0]);
		
	}
	sleep(5);
	
	// Recieve ACKS
	// Nodes wait for ACKs from neighbours here
	count = 0;
	while(count < neighbours)  {
		//printf("Here!\n");
		if ((numbytes = recvfrom(sockfd1, buffer, 4 , 0,
			(struct sockaddr *)&their_addr1, &addr_len1)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		memcpy(&translated_int1, buffer, sizeof translated_int1);
		original_int1 = ntohl(translated_int1);


		//printf("Recieved init request! %d\n", original_int1);
		memset(message, 0, sizeof message);
		sprintf(message, "Received ACK from router %d\n", original_int1);
		logMessage(message, logFile);
		//fprintf(logFile, "Received ACK from router %d\n", original_int1);
		count++;
	}	
	
	// Send OKAY message to manager
	translated_int = htonl(9999);
	binary_string = (char *)&translated_int;
	if (send(sockfd, binary_string, 4, 0) == -1)  { 
		printf("send\n");
		close(sockfd);
		exit(0);
	}
	
	// Wait for go from manager to begin exchanging LSPs
	while(1)  {
		if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
			printf("\nError recieving message!\n");
			exit(1);
		}
		memcpy(&translated_int1, buffer, sizeof translated_int1);
		original_int1 = ntohl(translated_int1);
		int code = original_int1;
		if(code == 8888)  {
			memset(message, 0, sizeof message);
			sprintf(message, "Permission to initiate LSP exchange received from manager.\n");
			logMessage(message, logFile);
			break;
		}
	}
	sleep(3);
	string packed = packMessage(topologyTable, topologyRows, routerId);
	const char *des = packed.c_str();
	
	memset(message, 0, sizeof message);
	sprintf(message, "LSP: %s\n", des);
	logMessage(message, logFile);
	//fprintf(logFile, "%s",  des);
	
	// Start exchanging LSPs
	// Send you LSP to neighbours first
	for(int i = 0; i < neighbours; i++)  {
			int sock;
			struct addrinfo hints, *servinfo, *p;
			int rv;

			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_DGRAM;
			string s = to_string(topologyTable[i][2]);
			char const *pchar = s.c_str();  //use char const* as target type
			if ((rv = getaddrinfo(managerIp, pchar, &hints, &servinfo)) != 0) {
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
				return 1;
			}
			// loop through all the results and make a socket
			for(p = servinfo; p != NULL; p = p->ai_next) {
				if ((sock = socket(p->ai_family, p->ai_socktype,
					p->ai_protocol)) == -1) {
					perror("talker: socket");
					continue;
				}
				break;
			}
			if (p == NULL) {
				fprintf(stderr, "talker: failed to bind socket\n");
				return 2;
			}
			
			translated_int = htonl(routerId);
			binary_string = (char *)&translated_int;
			if ((numbytes = sendto(sock, des, packed.length(), 0,
				p->ai_addr, p->ai_addrlen)) == -1) {
				perror("talker: sendto");
				exit(1);
			}
			freeaddrinfo(servinfo);
			memset(message, 0, sizeof message);
			sprintf(message, "LSP sent to neighbour router %d\n", topologyTable[i][0]);
			logMessage(message, logFile);
			//fprintf(logFile, "Sent initialize link message to node %d\n", topologyTable[i][0]);
		
		}
		sleep(5);
	// Now recieve, and forward as necessary
	vector<string> recList;
	recList.push_back(packed);
	int c = 1;
	char messageBuffer[1024];
	struct sockaddr_in sender;
	socklen_t len = sizeof sender;
	while(c < routers)  {
		memset(messageBuffer, 0, sizeof messageBuffer);
		if ((numbytes = recvfrom(sockfd1, messageBuffer, 1024 , 0,
			(struct sockaddr *)&sender, &len)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		// check if its my own
		if(strcmp(recList[0].c_str(), messageBuffer) == 0)  {
			//printf("My own!\n");
			continue;
		} 
		// Check if we have already received it
		else if (std::find(recList.begin(), recList.end(), messageBuffer) != recList.end())	{
			// Element in vector.
			//Duplicate, do nothing
			//printf("Duplicate!\n");
			continue;
		}
		else  {
			recList.push_back(messageBuffer);
			memset(message, 0, sizeof message);
			sprintf(message, "LSP received: %s\n", recList[c].c_str());
			logMessage(message, logFile);
			c++;
			
			// Find remote port
			struct sockaddr_in* temp;
			temp = (struct sockaddr_in*)&sender;
			//memset(message, 0, sizeof message);
			//sprintf(message, "Remote port: %hu", temp->sin_port);
			//logMessage(message, logFile);
			
			// Limited Broadcast
			for(int i = 0; i < neighbours; i++)  {
				int sock;
				struct addrinfo hints, *servinfo, *p;
				int rv;

				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_DGRAM;
				string s = to_string(topologyTable[i][2]);
				char const *pchar = s.c_str();  //use char const* as target type
				if ((rv = getaddrinfo(managerIp, pchar, &hints, &servinfo)) != 0) {
					fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
					return 1;
				}
				// loop through all the results and make a socket
				for(p = servinfo; p != NULL; p = p->ai_next) {
					if ((sock = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
						perror("talker: socket");
						continue;
					}
					break;
				}
				if (p == NULL) {
					fprintf(stderr, "talker: failed to bind socket\n");
					return 2;
				}
				
				//translated_int = htonl(routerId);
				//binary_string = (char *)&translated_int;
				if ((numbytes = sendto(sock, messageBuffer, strlen(messageBuffer), 0,
					p->ai_addr, p->ai_addrlen)) == -1) {
					perror("talker: sendto");
					exit(1);
				}
				freeaddrinfo(servinfo);
				memset(message, 0, sizeof message);
				sprintf(message, "LSP sent to neighbour router %d\n", topologyTable[i][0]);
				logMessage(message, logFile);
				//fprintf(logFile, "Sent initialize link message to node %d\n", topologyTable[i][0]);
			
			}
		}
	}
	memset(message, 0, sizeof message);
	sprintf(message, "All LSPs received!\n");
	logMessage(message, logFile);
	
	// Reconstruct the topology table
	int** topology = (int**)malloc(routers * routers * sizeof(int*));
	int *temp;
	temp = (int*)malloc((routers * routers) * 3 * sizeof(int));
	for (int i = 0; i < routers * routers; i++) {
	  topology[i] = temp + (i * 3);
	  //printf("%d\n", i);
	}
	
	int networkRows = 0;
	for(int j = 0; j < recList.size(); j++)  {
		istringstream iss(recList[j]);
		vector<string> tokens;
		copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter<vector<string> >(tokens));
         
         int node = atoi(tokens[1].c_str());
         int count = atoi(tokens[2].c_str());
         int place = 3;
         
         for(int i = 0; i < count; i++)  {
			topology[networkRows][0] = node;
			topology[networkRows][1] = atoi(tokens[place].c_str());
			topology[networkRows][2] = atoi(tokens[place + 1].c_str());
			place+=3;
			networkRows++;
		 }
	}
	memset(message, 0, sizeof message);
	sprintf(message, "\nNetwork Graph:\n");
	logMessage(message, logFile);
	for(int i = 0; i < networkRows; i++)  {
		fprintf(logFile, " %d %d %d\n", topology[i][0], topology[i][1], topology[i][2]);
	}
	fprintf(logFile, "\n");
	
	// Routing Table
	int** routingTable;
	int* temp1;
	int routingRows = 0;
	routingTable = (int**)malloc(routers * sizeof(int*));
	temp1 = (int*)malloc((routers) * 2 * sizeof(int));
	for (int i = 0; i < routers; i++) {
	  routingTable[i] = temp1 + (i * 2);
	  //printf("%d\n", i);
	}
	
	// Construct the graph for use in Dijkstra's Algorithm
	adjacency_list_t adjacency_list(routers);
	
	for(int i = 0; i < routers; i++)  {
		for(int j = 0; j < networkRows; j++)  {
			if(topology[j][0] == i)  {
				adjacency_list[i].push_back(neighbor(topology[j][1], topology[j][2]));
			}
		}
	}
	
	std::vector<weight_t> min_distance;
    std::vector<vertex_t> previous;
    
    memset(message, 0, sizeof message);
	sprintf(message, "\nSPT:");
	logMessage(message, logFile);
	memset(message, 0, sizeof message);
    
    // Compute SPT
    for(int i = 0; i < routers; i++)  {
		if(i == routerId)  {
			fprintf(logFile, "%d : 0\n", routerId);
			routingTable[i][0] = i;
			routingTable[i][1] = -1;
			continue;
		}
		
		
		
		DijkstraComputePaths(routerId, adjacency_list, min_distance, previous);
		//sprintf(message, "Distance from %d to %d: %f\n", routerId, i, (min_distance[i]));
		//logMessage(message, logFile);
		memset(message, 0, sizeof message);
		
		list<vertex_t> path = DijkstraGetShortestPathTo(i, previous);
		stringstream s;
		copy(path.begin(), path.end(), ostream_iterator<vertex_t>(s, " "));
		sprintf(message, "%s: %d\n", s.str().c_str(), (int)min_distance[i]);
		
		//logMessage(message, logFile);
		fprintf(logFile, "%s", message);
		memset(message, 0, sizeof message);
		
		// Update Routing Table
		routingTable[i][0] = i;
		string ss = s.str();
		routingTable[i][1] = atoi(ss.substr(2, 1).c_str());
	}
	fprintf(logFile, "\n");
	
	// Output Forwarding table
	memset(message, 0, sizeof message);
	sprintf(message, "\nForwarding Table:\nRouter Sendto");
	logMessage(message, logFile);
	for(int i = 0; i < routers; i++)  {
		fprintf(logFile, "%d %d\n", routingTable[i][0], routingTable[i][1]);
	}
	fprintf(logFile, "\n");
	
	// Send an ok message to manager = 3333
	translated_int = htonl(3333);
	binary_string = (char *)&translated_int;
	if (send(sockfd, binary_string, 4, 0) == -1)  { 
					printf("send\n");
					close(sockfd);
					exit(0);
	}
	memset(message, 0, sizeof message);
	sprintf(message, "Ready to accept message permission sent to manager.\n");
	logMessage(message, logFile);
	
	
	// Setup the select call to listen to both the manager and neighbours
	fd_set active_fd_set, read_fd_set;
    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);
    FD_SET (sockfd1, &active_fd_set);
    
     struct sockaddr_in clientname;
    socklen_t *sin_size = (socklen_t*)malloc(sizeof(socklen_t));
	//*sin_size = sizeof their_addr;
	
	bool cont = true;
	while (cont)  {
		/* Block until input arrives on one or more active sockets. */
		FD_ZERO (&read_fd_set);
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
			//printf("We're here!\n");
			if (FD_ISSET (i, &read_fd_set))
			{

				// Message on TCP socket
				if (sockfd == i)
				{
					if ((numbytes = recv(sockfd, buffer, 4, 0)) == -1) {
						printf("\nError recieving message!\n");
						exit(1);
					}
					memcpy(&translated_int1, buffer, sizeof translated_int1);
					original_int1 = ntohl(translated_int1);
					
					// Quit message
					if(original_int1 == 6789)  {
						memset(message, 0, sizeof message);
						sprintf(message, "Kill signal received. Exiting...\n");
						logMessage(message, logFile);
						cont = false;
						break;
					}
					// Data to send
					uint32_t data = original_int1;
					memset(message, 0, sizeof message);
					sprintf(message, "Data Paket to send: %d\n", original_int1);
					logMessage(message, logFile);
					//printf("Data to send:%d", data);
					//cont = false;
					
					// Send data to router
					// Send it to the appropriate neighbour
					int sock;
					struct addrinfo hints, *servinfo, *p;
					int rv;

					memset(&hints, 0, sizeof hints);
					hints.ai_family = AF_UNSPEC;
					hints.ai_socktype = SOCK_DGRAM;
					
					int destination;
					// find the next hop
					for(int k = 0; k < routers; k++)  {
						if(routingTable[k][0] == original_int1)  {
							destination = routingTable[k][1];
						}
					}
					//cout << "Next hop:" << destination << endl;
					string s;
					// find the UDP port
					for(int k = 0; k < topologyRows; k++)  {
						if(topologyTable[k][0] == destination)  {
							s = to_string(topologyTable[k][2]);
						}
					}
					//cout << "UDP of hop:" << s << endl;
					
					
					//string s = to_string(topologyTable[i][2]);
					char const *pchar = s.c_str();  //use char const* as target type
					if ((rv = getaddrinfo(managerIp, pchar, &hints, &servinfo)) != 0) {
						fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
						return 1;
					}
					// loop through all the results and make a socket
					for(p = servinfo; p != NULL; p = p->ai_next) {
						if ((sock = socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
							perror("talker: socket");
							continue;
						}
						break;
					}
					if (p == NULL) {
						fprintf(stderr, "talker: failed to bind socket\n");
						return 2;
					}
					
					//translated_int = htonl(routerId);
					//binary_string = (char *)&translated_int;
					if ((numbytes = sendto(sock, buffer, 4, 0,
						p->ai_addr, p->ai_addrlen)) == -1) {
						perror("talker: sendto");
						exit(1);
					}
					freeaddrinfo(servinfo);
					memset(message, 0, sizeof message);
					sprintf(message, "Forwarding data Packet %d to router %d\n", original_int1, destination);
					logMessage(message, logFile);
					break;
				}
				// Message on UDP socket
				else if(sockfd1 == i) {
					char mBuffer[4];
					memset(mBuffer, 0, sizeof mBuffer);
					if ((numbytes = recvfrom(sockfd1, mBuffer, 4 , 0,
						(struct sockaddr *)&sender, &len)) == -1) {
						perror("recvfrom");
						exit(1);
					}
					memcpy(&translated_int1, mBuffer, sizeof translated_int1);
					original_int1 = ntohl(translated_int1);
					//cout << "UDP read " << original_int1 << endl;
					if(original_int1 > routers)
						break;
					
					// Check if its destination is me
					if(original_int1 == routerId)  {
						cout << "\nPacket " << original_int1 << " received at destination!" << endl;
						memset(message, 0, sizeof message);
						sprintf(message, "**My Packet** Receieved data Packet %d\n", original_int1);
						logMessage(message, logFile);
						break;
					}
					
					memset(message, 0, sizeof message);
					sprintf(message, "Receieved data Packet %d\n", original_int1);
					logMessage(message, logFile);
					
					// Send it to the appropriate neighbour
					int sock;
					struct addrinfo hints, *servinfo, *p;
					int rv;

					memset(&hints, 0, sizeof hints);
					hints.ai_family = AF_UNSPEC;
					hints.ai_socktype = SOCK_DGRAM;
					
					int destination;
					// find the next hop
					for(int k = 0; k < routers; k++)  {
						if(routingTable[k][0] == original_int1)  {
							destination = routingTable[k][1];
						}
					}
					//cout << "Next hop:" << destination << endl;
					string s;
					// find the UDP port
					for(int k = 0; k < topologyRows; k++)  {
						if(topologyTable[k][0] == destination)  {
							s = to_string(topologyTable[k][2]);
						}
					}
					//cout << "UDP of hop:" << s << endl;
					
					//string s = to_string(topologyTable[i][2]);
					char const *pchar = s.c_str();  //use char const* as target type
					if ((rv = getaddrinfo(managerIp, pchar, &hints, &servinfo)) != 0) {
						fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
						return 1;
					}
					// loop through all the results and make a socket
					for(p = servinfo; p != NULL; p = p->ai_next) {
						if ((sock = socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
							perror("talker: socket");
							continue;
						}
						break;
					}
					if (p == NULL) {
						fprintf(stderr, "talker: failed to bind socket\n");
						return 2;
					}
					
					//translated_int = htonl(routerId);
					//binary_string = (char *)&translated_int;
					if ((numbytes = sendto(sock, mBuffer, 4, 0,
						p->ai_addr, p->ai_addrlen)) == -1) {
						perror("talker: sendto");
						exit(1);
					}
					freeaddrinfo(servinfo);
					memset(message, 0, sizeof message);
					sprintf(message, "Forwarding data Packet %d to router %d\n", original_int1, destination);
					logMessage(message, logFile);
					break;
				}
			}
		}
	}
	printf("Router %d is exiting...\n", routerId);
	return 0;
}


