#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <string.h>

#define MAX_BUF_SIZE 256
/* Change ROW and COL when changing the size of the graph */
#define ROW 5
#define COL 5

int main(int argc, char *argv[])
{

	int sockfd, portnum, n,node_id,i;

	int row_size = COL*sizeof(char);

	struct sockaddr_in server_addr;

    char msgbuff[MAX_BUF_SIZE];
	char matlinebuff[MAX_BUF_SIZE];
	char retlinebuff[MAX_BUF_SIZE];
	char* matrix[ROW];
	
	/* Allocating memory for storing adjacency matrix */

	for(i=0; i<ROW; i++)
	{
		matrix[i] = malloc(MAX_BUF_SIZE);
	}

    if (argc < 4) 
	{
       fprintf(stderr,"%d : CLIENT: usage %s <server-ip-addr> <server-port> <node_id>\n",__LINE__, argv[0]);
       exit(0);
    }

	/* Converting 2nd and 3rd Command Line arguments into port numbers */

	portnum = atoi(argv[2]);
	node_id = atoi(argv[3]);

    /* Create client socket */
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
		fprintf(stderr, "%d : CLIENT: ERROR opening socket\n",__LINE__);
		exit(1);
    }

    /* Fill in server address */

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    
	if(!inet_aton(argv[1], &server_addr.sin_addr))
	{
		fprintf(stderr, "%d : CLIENT: ERROR invalid Blackboard IP address\n",__LINE__);
		exit(1);
    }
    server_addr.sin_port = htons(portnum);

    /* Connect to server */
    if (connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) 
    {
		fprintf(stderr, "%d : CLIENT (%d) : ERROR connecting to Blackboard\n", __LINE__, sockfd);
		exit(1);
    }
    printf("CLIENT (%d) : Connected to Blackboard\n", sockfd);
    
	/* Ask blackboard for Graph representation */

	strcpy(msgbuff,"get\n");
	
	n = write(sockfd,msgbuff,strlen(msgbuff));
	if (n < 0) 
	{
		fprintf(stderr, "%d : CLIENT (%d) : ERROR writing to socket\n", __LINE__, sockfd);
		exit(1);
	}

	/* Reading the problem instance sent by blackboard */

	/* Read the target search key from Blackboard */

	bzero(msgbuff,MAX_BUF_SIZE);
	
	char target[MAX_BUF_SIZE];	
	n = read(sockfd,msgbuff,(MAX_BUF_SIZE-1));
	if (n <= 0) 
	{
		fprintf(stderr, "%d : CLIENT (%d) : ERROR reading from socket\n", __LINE__, sockfd);
		exit(1);
	}
	if(matlinebuff[0] == 'B' && matlinebuff[1] == 'A' && matlinebuff[2] == 'D')
	{
		fprintf(stderr, "%d : CLIENT (%d) : ERROR: wrong reply from blackboard\n", __LINE__, sockfd);
		exit(1);
	}
	
	printf("CLIENT (%d) : Target is : %s\n", sockfd, msgbuff);
	strcpy(target,msgbuff);

	/* Read the keys set from Blackboard*/
	
	bzero(matlinebuff,MAX_BUF_SIZE);
	
	char keys[MAX_BUF_SIZE];	
	n = read(sockfd,matlinebuff,(MAX_BUF_SIZE-1));
	if (n <= 0) 
	{
		fprintf(stderr, "%d : CLIENT (%d) : ERROR reading from socket\n", __LINE__, sockfd);
		exit(1);
	}
	if(matlinebuff[0] == 'B' && matlinebuff[1] == 'A' && matlinebuff[2] == 'D')
	{
		fprintf(stderr, "%d : CLIENT (%d) : ERROR: wrong reply from blackboard\n", __LINE__, sockfd);
		exit(1);
	}
	
	printf("CLIENT (%d) : Set of keys: %s\n", sockfd, matlinebuff);
	strcpy(keys,matlinebuff);

	/* Read the matrix sent by Blackboard*/
	
	for(i=0; i<ROW; i++)
	{
		memset(matlinebuff,0,MAX_BUF_SIZE);
		n = read(sockfd,matlinebuff,(MAX_BUF_SIZE-1));
		if (n <= 0) 
		{
			fprintf(stderr, "%d : CLIENT (%d) : ERROR reading from socket\n", __LINE__, sockfd);
			exit(1);
		}
		if(matlinebuff[0] == 'B' && matlinebuff[1] == 'A' && matlinebuff[2] == 'D')
		{
			fprintf(stderr, "%d : CLIENT (%d) : ERROR: wrong reply from blackboard\n", __LINE__, sockfd);
			exit(1);
		}	
		printf("CLIENT (%d) : Matrix line %d: %s\n",sockfd,i,matlinebuff);
		strcpy(matrix[i], matlinebuff);
	}

	/* Process and find the neighbours */

	char adjlist[MAX_BUF_SIZE];
	char result[MAX_BUF_SIZE];
	strcpy(adjlist,matrix[node_id]);
	memset(result,0,MAX_BUF_SIZE);
	
	int found = 0;
	
	if(target[0] == keys[node_id])
	{
		strcpy(result,"fnd");
	}
	else
	{	
		for(i=0; i<MAX_BUF_SIZE; i++)
		{
			if(adjlist[i] == '\0')
			{
				break;
			}
			if(adjlist[i] == '1')
			{	
				if(target[0] != keys[i])			//when a neighbour's key!=target
				{			
					if(i<10)
					{
						sprintf(result,"%s0%d",result,i);
					}
					else
					{
						sprintf(result,"%s%d",result,i);
					}
				}
				else								//when neighbour's key = target
				{	
					memset(result,0,MAX_BUF_SIZE);
					if(i<10)
					{
						sprintf(result,"fnd0%d",i);
					}
					else
					{
						sprintf(result,"fnd%d",i);
					}
					found = 1;
					break;
				}
			}
		}
	}
		
	/* Write the result to the socket */	

	bzero(retlinebuff,MAX_BUF_SIZE);
	if(!found)
	{
		strcpy(retlinebuff,"res");
		strcat(retlinebuff,result);
	}
	else
	{
		strcpy(retlinebuff,result);
	}
		
	fprintf(stdout,"CLIENT (%d) : Result sent to %s\n",sockfd,retlinebuff);

	n = write(sockfd,retlinebuff,strlen(retlinebuff));
	if (n < 0) 
	{
		fprintf(stderr, "%d : CLIENT (%d) : ERROR writing to socket\n", __LINE__, sockfd);
		close(sockfd);
		exit(1);
	}

   	return 0;
}
