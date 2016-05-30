#include "bb.h"

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

/*send the problem instance to a client*/
void* send_problem_instance(struct arg_list* args)
{
	char hostname_string[MAX_BUF_SIZE];
	char msgbuff[MAX_BUF_SIZE];
	int connected_sockfd = args->fd_sock;
	int return_val;
	int i=0;

	strcpy(hostname_string ,args->name_string);	
	
	//fprintf(stdout," *** | Testing: Hostname: \"%s\" Socket: %d *** \n",hostname_string, connected_sockfd);
	memset(msgbuff,0,MAX_BUF_SIZE);
	strcpy(msgbuff, target);

	/*send the key to be searched for*/
	return_val = send(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),ZERO);
	if (return_val <=  ZERO)
	{
		//INFO(HELLO);
		fprintf(stderr, "bb child: ERROR: send(): while sending from connected client socket: %d: %s: %d !!\n",
				connected_sockfd, __FILE__, __LINE__);
		perror("");
		close(connected_sockfd);
		return NULL;
	}

	memset(msgbuff,0,MAX_BUF_SIZE);
	strcpy(msgbuff, keys);

	/*send the key values of graph nodes*/
	return_val = send(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),ZERO);
	if (return_val <=  ZERO)
	{
		//INFO(HELLO);
		fprintf(stderr, "bb child: ERROR: send(): while sending from connected client socket: %d: %s: %d !!\n",
				connected_sockfd, __FILE__, __LINE__);
		perror("");
		close(connected_sockfd);
		return NULL;
	}

	for(i=0;i<ROW;i++)
	{
		memset(msgbuff,0,MAX_BUF_SIZE);
		strcpy(msgbuff, matrix[i]);
		
		//fprintf(stdout,"matrix[%d]: %s \n",i, matrix[i]);
		fprintf(stdout,"sending: %s \n",msgbuff);

		/*send the adj matrix row of each node*/
		return_val = send(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),ZERO);
		if (return_val <=  ZERO)
		{
			//INFO(HELLO);
			fprintf(stderr, "bb child: ERROR: send(): while sending from connected client socket: %d: %s: %d !!\n",
					connected_sockfd, __FILE__, __LINE__);
			perror("");
			close(connected_sockfd);
			return NULL;
		}
	}
}

/*get the adj matrix of graph from graph.txt*/
void setup_graph()
{
	FILE* stream = fopen("graph.txt", "r");

	char line[1024];
	int line_no = 0;
	int i=0;
	
	for(i=0; i<ROW; i++)
	{
		matrix[i] = malloc(COL*sizeof(char));
	}

	while (fgets(line, 1024, stream) && line_no < ROW)
	{
		int j=0;

		for(j=0; j<COL; j++)
		{
			char* tmp = strdup(line);
			strcat(matrix[line_no], getfield(tmp,j+1));
			free(tmp);
		}
		line_no++;
	}
	for(i=0; i<ROW; i++)
	{
		fprintf(stdout,"bb parent : %d: %s \n",i,matrix[i]);
	}
}

/*get the key values of graph nodes from keys.txt*/
void setup_keys()
{
	FILE* stream = fopen("keys.txt", "r");

	char line[1024];
	int i=0;
	
	keys = malloc(NKEYS*sizeof(char));

	while (fgets(line, 1024, stream))
	{
		int j=0;

		for(j=0; j<NKEYS; j++)
		{
			char* tmp = strdup(line);
			strcat(keys,getfield(tmp,j+1));
			free(tmp);
		}
	}
	fprintf(stdout,"bb parent : keys: %s \n",keys);
}

/*get the search value from target.txt*/
void setup_target()
{
	FILE* stream = fopen("target.txt", "r");

	char line[1024];
	int i=0;
	
	target = malloc(1*sizeof(char));

	while (fgets(line, 1024, stream))
	{
		char* tmp = strdup(line);
		strcpy(target, getfield(tmp,1));
		free(tmp);
	}
	fprintf(stdout,"bb parent : Target: %s \n",target);
}

/*set up problem instance from files*/
int setup_problem()
{
	/*get the search value*/
	setup_target();
	/*get the key values of graph nodes*/
	setup_keys();
	/*get the adj matrix of graph*/
	setup_graph();
}

/*display ip on interface*/
void display_ip_on_interface(struct sockaddr_in* serv1_sockaddr_in)
{
	/*Code obtained from net to print IP of interface*/
	int ret;
	struct ifreq ifr;
	ret = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, IFACE, IFNAMSIZ-1);				//specify the interface on which you are listening e.g. eth0, wlan0 ...
	ioctl(ret, SIOCGIFADDR, &ifr);
	fprintf(stdout,"\t\t\tAF:  %d\n\n", serv1_sockaddr_in->sin_family);
	fprintf(stdout,"\t\t\tPort:  %d\n\n",ntohs(serv1_sockaddr_in->sin_port));
	//fprintf(stdout,"\t\t\tIP:  %s\n\n",ip_address_string);
	fprintf(stdout,"\t\t\tIP:  %s\n\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	close(ret);
	/* end of code from net.                          */
}

/*socket attribute setup*/
void setup_socket(struct sockaddr_in* sockaddr_in, int port)
{
	sockaddr_in->sin_family = AF_INET;	
	sockaddr_in->sin_port = htons(port);
	sockaddr_in->sin_addr.s_addr = INADDR_ANY;	
}

/*dequeue result to controller*/
void dequeue_result(char* msgbuff)
{
	struct msgqstruct buf;
	if(msgrcv(msgqid, &buf, MAX_BUF_SIZE, 0, 0) == -1) 
	{
		perror("msgrcv");
	}
	strcpy(msgbuff, buf.qbuff);
}

/*Handle communication with ctrl*/
void* handle_ctrl(void* args)
{
	char msgbuff[MAX_BUF_SIZE];
	int ret_flag = ZERO;
	int i = 0;  
	int return_val;
	char hostname_string[MAX_BUF_SIZE];
	struct arg_list* new_args = (struct arg_list*)args;
	int connected_sockfd = new_args->fd_sock;
	char* matrix[ROW];
	char* keys[NKEYS];

	strcpy(hostname_string ,new_args->name_string);	

	/*
	 * admit only a single ctrl
	 * check if a ctrl already exists
	 */
	if(CTRL_IN == 0)
	{
		CTRL_IN = 1;
		fprintf(stdout,"bb parent: Controller In\n");
	}
	else
	{
		ret_flag = 1;
		fprintf(stdout,"bb parent: Controller already exists\n");
		memset(msgbuff,0,MAX_BUF_SIZE);
		strcpy(msgbuff, "Controller already exists\n");
		return_val = send(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),ZERO);
		if (return_val <=  ZERO)
		{
			fprintf(stderr, "bb parent: ERROR: send(): while sending from connected dubious ctrl socket: %d: %s: %d !!\n",
					connected_sockfd, __FILE__,__LINE__);
			perror("");
			close(connected_sockfd);
			return NULL;
		}
	}

	while(!ret_flag)
	{

		memset(msgbuff,0,MAX_BUF_SIZE);

		/*receive from controller*/
		return_val = recv(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),0);

		if (return_val <=  ZERO) 
		{
			fprintf(stderr, "bb parent: ERROR: recv(): while reading from connected ctrl socket: %d: %s: %d !!\n",
					connected_sockfd, __FILE__,__LINE__);
			perror("");
			close(connected_sockfd);
			return NULL;
		}

		
		fprintf(stdout,"bb parent: ctrl \"%s\" connected on socket  %d says: %s\n", hostname_string, connected_sockfd, msgbuff);

		/*respond to only get msg queue for now*/
		if((msgbuff[0] == 'g' && msgbuff[1] == 'e' && msgbuff[2] == 't' && msgbuff[3] =='\n'))
		{
			memset(msgbuff,0,MAX_BUF_SIZE);

			/*dequeue client response from msg queue*/
			dequeue_result(msgbuff);
			fprintf(stdout,"bb parent: result with ctrl: %s \n", msgbuff);
		}
		/* I added this */
		else if((msgbuff[0] == 'O' && msgbuff[1] == 'K' && msgbuff[2] =='\n'))
		{
			ret_flag = 1;
			fprintf(stdout,"bb parent: ctrl \"%s\" connected on socket  %d exiting!!: %s\n", hostname_string, connected_sockfd, msgbuff);
			CTRL_IN = 0;
		} 
		else
		/*no response to anything invalid*/
		{
			fprintf(stdout,"bb parent: ctrl said something invalid: \"%s\" : %s: %d \n\n", msgbuff,__FILE__, __LINE__);
			continue;
		}

		if(!ret_flag)
		{
			/*send the client response obtained from msg queue to ctrl*/
			return_val = send(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),ZERO);
			if (return_val <=  ZERO)
			{
				fprintf(stderr, "bb parent: ERROR: shutdown() while shutting down connected ctrl socket: %d: %s: %d !!\n",
						connected_sockfd, __FILE__, __LINE__);
				perror("");
				close(connected_sockfd);
				return NULL;
			}
		}
	}

	/*SHUT_RDWR-Disabling both Tx and Rx on the socket*/
	if(shutdown(connected_sockfd, SHUT_RDWR) < ZERO)
	{
		fprintf(stderr,"bb parent: Error: shutdown() couldn't shutdown connected ctrl socket: %d: %s: %d !!\n",
				connected_sockfd,__FILE__, __LINE__);
		perror("");
		close(connected_sockfd);
		return NULL;

	}
	close(connected_sockfd);
	return NULL;
}

/*send error msg to client on receiving something unexpected before closing connection*/
void* send_bad_to_client(int connected_sockfd)
{
	char msgbuff[MAX_BUF_SIZE];
	int return_val;

	fprintf(stdout,"bb child: Something unexpected on client socket: %d\n\n", connected_sockfd);
	memset(msgbuff,0,MAX_BUF_SIZE);
	strcpy(msgbuff, "BAD");
	return_val = send(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),ZERO);

	if (return_val <=  ZERO)
	{
		perror("");
		fprintf(stderr, "bb child: ERROR: send(): while sending from connected client socket: %d: %s: %d !!\n",
			   	connected_sockfd, __FILE__, __LINE__);
		close(connected_sockfd);
		return NULL;
	}
	if(shutdown(connected_sockfd, SHUT_RDWR) < ZERO)
	{
		fprintf(stderr, "bb child: ERROR: shutdown() while shutting down connected client socket: %d: %s: %d !!\n",
			   	connected_sockfd, __FILE__, __LINE__);
		perror("");
		close(connected_sockfd);
		return NULL;
	}
}

/*enqueuing into message queue from client*/
void enqueue_result(char *result)
{
	struct msgqstruct res_struct;
	res_struct.mtype = 1;

	strcpy(res_struct.qbuff, result);

	if((msgsnd(msgqid, &res_struct, MAX_BUF_SIZE, 0))==-1)
	{
		perror("msgsnd");
	}

}

/*Handle communication with client*/
void* handle_client(void* args)
{
	char msgbuff[MAX_BUF_SIZE];
	int ret_flag = ZERO;
	int i = 0;  
	int return_val;
	char hostname_string[MAX_BUF_SIZE];
	struct arg_list* new_args = (struct arg_list*)args;
	int connected_sockfd = new_args->fd_sock;

	strcpy(hostname_string ,new_args->name_string);	

	//while(!ret_flag)
	//{

		memset(msgbuff,0,MAX_BUF_SIZE);

		/*receive request from client*/
		return_val = recv(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),0);
		if (return_val <=  ZERO) 
		{
			perror("");
			fprintf(stderr, "bb child: ERROR: recv(): while reading from connected client socket: %d: %s: %d !!\n",
					connected_sockfd,__FILE__, __LINE__);
			close(connected_sockfd);
			return NULL;
		}

		/*check if it is a valid "get"*/	
		fprintf(stdout,"bb child: Client \"%s\" connected on socket  %d says: %s\n", hostname_string, connected_sockfd, msgbuff);
		if((msgbuff[0] == 'g' && msgbuff[1] == 'e' && msgbuff[2] == 't' && msgbuff[3] =='\n'))
		{
			fprintf(stdout,"bb child: Received get from client: %s  on socket: %d \n\n", hostname_string, connected_sockfd);

			/*send adj matrix, search node, keys*/
			send_problem_instance(new_args);
		}
		else
		{
			send_bad_to_client(connected_sockfd);
		}

		/*get result from client*/
		return_val = recv(connected_sockfd,msgbuff,(MAX_BUF_SIZE-1),0);
		if (return_val <=  ZERO) 
		{
			fprintf(stderr, "bb child: ERROR: recv(): while reading from connected client socket: %d: %s: %d !!\n",
					connected_sockfd, __FILE__, __LINE__);
			perror("");
			close(connected_sockfd);
			return NULL;
		}

		/*check if valid "res"*/
		if((msgbuff[0] == 'r' && msgbuff[1] == 'e' && msgbuff[2] == 's'))
		{

			fprintf(stdout,"bb child: Received res: %s from client: %s  on socket: %d \n\n", msgbuff, hostname_string, connected_sockfd);
			
			/*enqueue result in msg queue*/	
			enqueue_result(msgbuff);

			/*SHUT_RDWR-Disabling both Tx and Rx on the socket*/
			if(shutdown(connected_sockfd, SHUT_RDWR) < ZERO)
			{
				fprintf(stderr,"bb child: Error: shutdown() couldn't shutdown connected socket: %s: %d !!\n\n\n\n", __FILE__, __LINE__);
				perror("");
				close(connected_sockfd);
				return NULL;

			}
			close(connected_sockfd);
			return NULL;

		}
		/*check if valid "fnd": search key found*/
		else if((msgbuff[0] == 'f' && msgbuff[1] == 'n' && msgbuff[2] == 'd'))
		{

			fprintf(stdout,"bb child: Received fnd: %s from client: %s  on socket: %d \n\n", msgbuff, hostname_string, connected_sockfd);
			
			/*enqueue result node*/
			enqueue_result(msgbuff);

			/*SHUT_RDWR-Disabling both Tx and Rx on the socket*/
			if(shutdown(connected_sockfd, SHUT_RDWR) < ZERO)
			{
				fprintf(stderr,"bb child: Error: shutdown() couldn't shutdown connected socket: %s: %d !!\n\n\n\n", __FILE__, __LINE__);
				perror("");
				close(connected_sockfd);
				return NULL;

			}
			close(connected_sockfd);
			return NULL;

		}
		else
		{
			send_bad_to_client(connected_sockfd);
		}

	//}

}

/*
 * setup connection with clients or controller.
 * bb parent will handle connection with controller in a multithreaded way; but will allow only 1 sole controller.
 * id = 0 for bb parent.
 * bb child will handle connection with clients in a multithreaded way. id = 1 for bb child.
 */
int setup_connection_on_socket(struct sockaddr_in* sockaddr_in,  int id, char** argv)
{
	int sockfd = MINUS_ONE;
	int length = MINUS_ONE; 		//var holding length of anything --typically a string
	int error_number = MINUS_ONE;
	pthread_t new_client;
	pthread_t new_ctrl;

	
	int connected_sockfd = MINUS_ONE;

	/****************************************************/
	/*** Start:Code common for both bb parent & child ***/
	/****************************************************/ 
	/*create socket*/
	sockfd = socket(sockaddr_in->sin_family, SOCK_STREAM, ZERO); //0 specifies the default protocol for the specified socket.
	
	if(sockfd < ONE)
	{
		fprintf(stderr,"bb: Error: socket(): \"%s\" couldn't open listening socket!!: %s: %d \n\n\n\n",
				((*argv[0]=='.')?(argv[0]+2):(argv[0])), __FILE__, __LINE__);
		perror("");
		close(sockfd);
		return (1);
	}

	/*bind socket*/
	if(bind(sockfd, (struct sockaddr *)sockaddr_in, sizeof(*sockaddr_in)) < ZERO)
	{

		fprintf(stderr,"Error: bind() \"%s\" couldn't bind listening socket!!: %s: %d \n\n\n\n",
				((*argv[0]=='.')?(argv[0]+2):(argv[0])), __FILE__, __LINE__);
		perror("");
		close(sockfd);
		return (1);
	}

	/*start listening*/
	if(listen(sockfd, 0) < ZERO)
	{
		fprintf(stderr,"bb: Error: listen() \"%s\" couldn't start listening on socket!!: %s: %d \n\n\n\n",
				((*argv[0]=='.')?(argv[0]+2):(argv[0])), __FILE__, __LINE__);
		perror("");
		//close(serv1_sockfd);
		return (1);
	}
	

	while (1)
	{
		struct connection_info* cinfo = (struct connection_info*)malloc(sizeof(struct connection_info));
		struct sockaddr_in connected_sockaddr_in;
		int length = sizeof(connected_sockaddr_in);
	
		if(!(cinfo->hostname_string = malloc(MAX_BUF_SIZE-1)))
		{ 
			fprintf(stderr,"\n\nbb: Error: malloc(): %s %d !!\n\n", __FILE__, __LINE__); 
			perror("");
			return (1); 
		}
		if(!(cinfo->ip_address_string = malloc(MAX_BUF_SIZE-1)))
		{ 
			fprintf(stderr,"\n\nbb: Error: malloc(): %s %d !!\n\n", __FILE__, __LINE__); 
			perror("");
			return (1); 
		}
		if(!(cinfo->service_name_str = malloc(MAX_BUF_SIZE-1)))
		{ 
			fprintf(stderr,"\n\nbb: Error: malloc(): %s %d !!\n\n", __FILE__, __LINE__); 
			perror("");
			return (1); 
		}

		/*accept incoming connections*/
		connected_sockfd = accept(sockfd, (struct sockaddr *) &connected_sockaddr_in, &length);

		if(connected_sockfd < ZERO )
		{
			fprintf(stderr,"bb: Error: accept() \"%s\" couldn't accept the connection request!!: %s: %d \n\n\n\n",
					((*argv[0]=='.')?(argv[0]+2):(argv[0])), __FILE__, __LINE__);
			perror("");
			close(sockfd);
			return (1);
		}

		/*get host name and ip of the incoming connection*/
		error_number = (getnameinfo((struct sockaddr *) &connected_sockaddr_in, sizeof(connected_sockaddr_in),
						cinfo->hostname_string,MAX_BUF_SIZE-1,cinfo->service_name_str,MAX_BUF_SIZE-1,0));
		if(error_number != ZERO)
		{
			
			fprintf(stderr,"bb: Error: %s : %s: %d !!\n\n", gai_strerror(error_number), __FILE__, __LINE__);
			close(connected_sockfd);
			close(sockfd);		
			return (0);
		}

		length = INET_ADDRSTRLEN;
		/*convert ip to system/host byte ordering*/
		if(!(inet_ntop(connected_sockaddr_in.sin_family,&(connected_sockaddr_in.sin_addr),cinfo->ip_address_string,length)))
		{ 
			fprintf(stderr,"bb: Error: inet_ntop(): %s: %d !!\n\n\n\n", __FILE__, __LINE__); 
			perror("");
		   	return (0); 
		}

		/*display info about accepted connection*/
		fprintf(stdout,"\t\t------------------------------------------------\n\n");
		fprintf(stdout,"\t\tClient: \"%s\" connected on socket: %d.\n\n", cinfo->hostname_string, connected_sockfd);
		fprintf(stdout,"\t\t%s IP: %s.\n\n", cinfo->hostname_string, cinfo->ip_address_string);
		fprintf(stdout,"\t\t%s running on: %d.\n\n", cinfo->hostname_string, ntohs(connected_sockaddr_in.sin_port));
		fprintf(stdout,"\t\t------------------------------------------------\n\n");

		/****************************************************/
		/*** End : Code common for both bb parent & child ***/
		/****************************************************/ 
		
		/*bb child: mutithreading for client processing*/
		if(id == 1)
		{
			struct arg_list* arguments = (struct arg_list*)malloc(sizeof(struct arg_list));
			int test;

			if(!(arguments->name_string = malloc(MAX_BUF_SIZE-1)))
			{ 
			
				fprintf(stderr,"\n\nbb child: Error: malloc(): %s %d !!\n\n", __FILE__, __LINE__); 
				perror("");
				return (1); 
			}
			arguments->fd_sock = connected_sockfd;	
			strcpy(arguments->name_string, cinfo->hostname_string);

			test = pthread_create(&new_client, NULL, handle_client, arguments);

			if(test != 0)
			{
				fprintf(stdout,"bb child: Error: pthread_create() \"%s\" couldn't create new client thread: %s: %d \n\n\n\n", 
						((*argv[0]=='.')?(argv[0]+2):(argv[0])), __FILE__, __LINE__);
			}
			pthread_detach(new_client);
		}
		/*bb parent: Multi threaded controller interaction*/
		else if(id == 0)
		{
			struct arg_list* arguments = (struct arg_list*)malloc(sizeof(struct arg_list));
			int test;

			if(!(arguments->name_string = malloc(MAX_BUF_SIZE-1)))
			{ 
				fprintf(stderr,"\n\nbb parent: Error: malloc(): %s %d !!\n\n", __FILE__, __LINE__); 
				perror("");
				return (1); 
			}
			arguments->fd_sock = connected_sockfd;	
			strcpy(arguments->name_string, cinfo->hostname_string);
			test = pthread_create(&new_ctrl, NULL, handle_ctrl, arguments);

			if(test != 0)
			{
				fprintf(stdout,"bb parent: Error: pthread_create() \"%s\" couldn't create new ctrl thread %s: %d \n\n\n\n", 
						((*argv[0]=='.')?(argv[0]+2):(argv[0])), __FILE__, __LINE__);
			}
			pthread_detach(new_ctrl);
		}
	}
}

/*                                                */
/*                     The main()                 */
/*                                                */

int main(int arg_count, char* argv[])
{
	int length = MINUS_ONE; //var holding length of anything --typically a string
	struct sockaddr_in ctrl_sockaddr_in;
	struct sockaddr_in client_sockaddr_in;
	char* hostname_string;
	char* ip_address_string;
	char* service_name_str;
	struct arg_list arguments;
	pid_t pid;

	int id; //0 - controller, 1 - client

	/*Check the no.of input arguments.   */

	int ret;

	if(arg_count!=3)
	{
		fprintf(stderr,"\n\nError: Usage: %s <Controller_Port_Number> <Client_port_number>!!\n\n", 
				((*argv[0]=='.')?(argv[0]+2):(argv[0])) );
		perror("");
		return (1);
	}


	/*Converting the 1st 2 arguments to port number       */
	CTRL_PORTNO = atoi(argv[1]);
	CLIENT_PORTNO = atoi(argv[2]);

#ifdef _SC_HOST_NAME_MAX
	length  = sysconf(_SC_HOST_NAME_MAX);
	/*Making sure var length gets max length value    */
	/*set for hostname.                               */
	if (length < ZERO)
#endif
		length = HOST_NAME_MAX;
	
	if(!(hostname_string = malloc(length)))
	{ 
		fprintf(stderr,"\n\nError: malloc() !!\n\n"); 
		perror("");
		return (1); 
	}

	/*Assigning memory to other imp strings.          */	
	if(!(service_name_str = malloc(MAX_BUF_SIZE-1)))
	{
		fprintf(stderr,"\n\nError: malloc() !!\n\n"); 
		//close(connected_sockfd); close(serv1_sockfd);
		perror("");
		return (1); 
	}
	
	if(!(ip_address_string = malloc(MAX_BUF_SIZE-1)))
	{
		fprintf(stderr,"\n\nError: malloc() !!\n\n"); 
		//close(connected_sockfd); close(serv1_sockfd);
		perror("");
		return (1); 
	}

	/*Obtain hostname using gethostname().            */
	if(gethostname(hostname_string, length) < ZERO)
	{
		fprintf(stderr,"\n\nError: gethostname() !!\n\n");
		perror("");

		return (1);
	}

	/*setiing up problem instance in blackboard by reading from file*/
	setup_problem();

	/*setting up socket for controller communication*/
	setup_socket(&ctrl_sockaddr_in, CTRL_PORTNO);
	
	fprintf(stdout,"\n\n\n\t\t---------------Welcome to Blackboard---------------\n\n");
 	fprintf(stdout,"\t\t------------------------------------------------\n\n");
	fprintf(stdout,"\t\t\tHost: %s \n\n", hostname_string);
 	//fprintf(stdout,"\t\t------------------------------------------------\n\n");
	
	/*displaying ip of connected interface*/
	display_ip_on_interface(&ctrl_sockaddr_in);

	fprintf(stdout,"\t\t------------------------------------------------\n\n");

	/*setting up socket for client communication*/
	setup_socket(&client_sockaddr_in, CLIENT_PORTNO);

	/*obtaining msg queue key*/
	if((msgqkey = ftok("./", 'a'))==-1)
	{
		perror("msgqkey");
		exit(1);
	}
	/*obtaining msg queue id*/
	if((msgqid = msgget(msgqkey, 0666 | IPC_CREAT))==-1)
	{
		perror("msgqid");
		exit(1);
	}

	/*enqueuing the start node:  00 - 1st node, 01- 2nd node, ...*/
	enqueue_result("res00");

	/*
	 * forking child process to handle multithreaded client communcation.
	 * parent will handle communication with controller.
	 */
	pid = fork();
	if(pid==0)
	{
		/*child: obtaining msg queue*/
		if((msgqid = msgget(msgqkey, 0666))==-1)
		{
			perror("child: msgqid");
			exit(1);
		}
		/*child: setting up socket for communication with clients*/
		ret = setup_connection_on_socket(&client_sockaddr_in, 1, argv);
	
		if(ret)
			return 1;
	}
	else
	{
		if(pid == -1)
		{	
			fprintf(stderr,"Error: fork() !!\n\n\n\n");
			perror("");
		}	
		/*parent: setting up socket for communication with controller*/
		ret = setup_connection_on_socket(&ctrl_sockaddr_in, 0, argv);
	
		if(ret)
			return 1;
	}

}
