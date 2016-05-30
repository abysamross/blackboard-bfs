#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define MAX_INPUT_SIZE 256
#define MAX_GP_SIZE 100
#define MAX_KEYS 99
#define NODE_ID_LEN 2 			// log MAX_KEYS to base 10
#define NET_STR 20

char result[MAX_INPUT_SIZE];
char map[MAX_GP_SIZE];
char bb_client_port[NET_STR];
char bb_ip[NET_STR];
char cmode[1] = "L";
char* passwd[MAX_KEYS];
char* host[MAX_KEYS];
char* path[MAX_KEYS];
int nhosts; // Number of hosts specified in trusted_hosts.txt

/*tokenize the fields of each line of an input csv file*/
const char* getfield(char* line, int num)
{
	const char* tok;
	for (tok = strtok(line, ";");tok && *tok;tok = strtok(NULL, ";\n"))
	{
		if (!--num)
		return tok;
	}
	return NULL;
}


/*get the information about remote hosts that can run BFS_client from trusted_hosts.txt*/
void gethost_info()
{
	FILE* stream = fopen("trusted_hosts.txt", "r");

	char line[1024];
	int line_no = 0;
	int i=0;
	
	for(i=0; i<MAX_KEYS; i++)
	{
		host[i] = malloc(MAX_INPUT_SIZE);
		passwd[i] = malloc(MAX_INPUT_SIZE);
		path[i] = malloc(MAX_INPUT_SIZE);
	}

	/* read and store the trusted hosts details */
	while (fgets(line, 1024, stream) && line_no < MAX_KEYS)
	{
		int j=0;

		char* tmp1 = strdup(line);
		char* tmp2 = strdup(line);
		char* tmp3 = strdup(line);

		strcpy(host[line_no], getfield(tmp1,1));
		strcpy(passwd[line_no], getfield(tmp2,2));
		strcpy(path[line_no], getfield(tmp3,3));

		free(tmp1);
		free(tmp2);
		free(tmp3);

		line_no++;
	}
	/*getting number of hosts specified in target.txt*/
	nhosts = line_no;
}

/*spawn a new client process either in remote or local host */
int spawn(char* program, char** arg_list)
{
	pid_t child_pid;

	child_pid = fork();
	if(child_pid != 0)
	{
		return child_pid;
	}
	else
	{
		int ret = 0;
		/* a child process exclusively spawned to start either a local or remote BFS_client  */
		ret = execvp(program, arg_list);	
		if(ret < 0)
		{
			fprintf(stderr,"CTRL : Spawn: Error: execvp returned !! : %s : %d\n", __FILE__, __LINE__);
			perror("");
		}
	}
}

/* check whether the node_id was previously explored and if not spawn a new process(local/remote) to explore it */ 
void check_map_and_spawn(char res[])
{
	int i=0;
	int node_id;
	int base = 10;
	int chosen_host=0;
	
	for(i=0; i<strlen(res)-1;i=i+NODE_ID_LEN)
	{
		/*reconstructing the node_id from res[] */
		node_id = ((res[i]-'0'))*base+((res[i+1])-'0');			//if MAX_KEYS changes, this needs to be fixed

    	fprintf(stdout,"CTRL : Node ID to be explored: %d \n", node_id);

		/* check if node_id already present in map */ 
		if(map[node_id]!=1)
		{
			char node[3];

			/* put node_id in map */
			map[node_id]=1;

			snprintf(node,sizeof(node),"%d",node_id);
    		fprintf(stdout,"CTRL : Node ID string being passed: %s \n", node);
    		
			/* Deciding whether to spawn a local or remote BFS_client */
			if(!strcmp(cmode,"L"))
			{
				char* arg_list[] = {"./BFS_client", bb_ip, bb_client_port, node,NULL};
				spawn("./BFS_client", arg_list);
			}
			else if(!strcmp(cmode,"R"))
			{
				char* arg_list[] = 
				{"sshpass", "-p", passwd[chosen_host], "ssh",host[chosen_host], path[chosen_host], bb_ip, bb_client_port, node,NULL};

				spawn("sshpass", arg_list);	
			}
			else
			{
				char* arg_list[] = {"./BFS_client", bb_ip, bb_client_port, node,NULL};
				spawn("./BFS_client", arg_list);
			}

			/*cycle among the hosts listed in trusted_hosts.txt*/
			chosen_host = (chosen_host+1)%nhosts;
		}
	}	
}

/*                                                */
/*                     The main()                 */
/*                                                */

int main(int argc, char *argv[])
{
    int sockfd, portnum, n;
    struct sockaddr_in server_addr;
    char inputbuf[MAX_INPUT_SIZE];
	memset(map,0,MAX_GP_SIZE);

    if (argc < 5) {
       fprintf(stderr,"CTRL : usage %s <bb-ip-addr> <bb-ctrl-port> <bb-client-port> <client type: L|R> : %s : %d \n", 
							argv[0], __FILE__, __LINE__);
       exit(0);
    }

	/* 
	 * storing bb port number(portnum), blackboard ip address(bb_ip), blackboard client communication port(bb_client_port), 
	 * client mode (local(L)/remote(R)) 
	 */
    portnum = atoi(argv[2]);
	strcpy(bb_client_port,argv[3]);
	strcpy(bb_ip, argv[1]);
	strcpy(cmode, argv[4]);

	/* getting trusted host information */
	gethost_info();

    /* Create client socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
      {
	fprintf(stderr, "CTRL : ERROR opening socket\n : %s : %d", __FILE__, __LINE__);
	exit(1);
      }

    /* Fill in server address */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if(!inet_aton(argv[1], &server_addr.sin_addr))

    {
	fprintf(stderr, "CTRL : ERROR Invalid IP address\n : %s : %d", __FILE__, __LINE__);
		exit(1);
    }
    server_addr.sin_port = htons(portnum);

    /* Connect to server */
    if (connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) 
    {
	fprintf(stderr, "CTRL : ERROR connecting\n : %s : %d", __FILE__, __LINE__);
		exit(1);
    }
    fprintf(stdout,"CTRL : Connected to server\n");
    
	
	/* loop for communicating with blackboard */	
    do
    {
		/* send a get request for queue status */
		bzero(inputbuf,MAX_INPUT_SIZE);
		strcpy(inputbuf,"get\n");
	
		/* Write to server */
		n = write(sockfd,inputbuf,strlen(inputbuf));
		if (n < 0) 
		{
			fprintf(stderr, "CTRL : ERROR writing to socket %d\n : %s : %d", sockfd, __FILE__, __LINE__);
			exit(1);
		}
	
		/* Read reply */
		bzero(inputbuf,MAX_INPUT_SIZE);
		n = read(sockfd,inputbuf,(MAX_INPUT_SIZE-1));
		if (n < 0) 
		{
			fprintf(stderr, "CTRL : ERROR reading from socket %d\n : %s : %d", sockfd, __FILE__, __LINE__);
			exit(1);
		}
		fprintf(stdout,"CTRL : Server replied: %s\n",inputbuf);

		/* Check that reply is either res or fnd */		
		
		if((inputbuf[0] == 'r' && inputbuf[1] == 'e' && inputbuf[2] == 's'))
		{
			snprintf(result,sizeof(result),"%s",inputbuf+3);
			fprintf(stdout, "CTRL : Blackboard says: %s \n", result);
			/* check the result nodes and spawn BFS_client if necessary */			
			check_map_and_spawn(result);
		}
		else if((inputbuf[0] == 'f' && inputbuf[1] == 'n' && inputbuf[2] == 'd'))
		{
			/* key was found */
			snprintf(result,sizeof(result),"%s",inputbuf+3);
			fprintf(stdout, "CTRL : Key was found on node with node_id : %s \n", result);
		}
	}while(!(inputbuf[0] == 'f' && inputbuf[1] == 'n' && inputbuf[2] == 'd'));
	
	/* Send an "OK" message to the blackboard */
	bzero(inputbuf,MAX_INPUT_SIZE);
	strcpy(inputbuf,"OK\n");

	// Write to server 
	n = write(sockfd,inputbuf,strlen(inputbuf));
	if (n < 0) 
	{
		fprintf(stderr, "CTRL : ERROR writing to a socket %d\n : %s : %d", sockfd, __FILE__, __LINE__);
		exit(1);
	}
	fprintf(stdout, "CTRL : Problem solved. Controller exits !!\n Waiting for unfinished clients... \n");
		
	/* Pausing for pending clients */
	sleep(5);	
	//fprintf(stdout, "CTRL : After sleep!!\n");

	/* closing controller connection with blackboard */		
	close(sockfd);	

	return 0;
}
