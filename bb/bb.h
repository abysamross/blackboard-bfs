#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>

/* Defining max size of host name                 */
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

#define MAX_BUF_SIZE 256

/* Change these values if problem instance changes*/
#define ROW 5
#define COL 5
#define NKEYS 5

/* change this value if interface changes */
#define IFACE "wlan0"

#define INFO(msg) \
	    fprintf(stderr, "info: %s: %s:%d: \n", #msg, __FILE__, __LINE__); \

enum vals {MINUS_ONE=-1,ZERO,ONE,TWO,THREE};

char*  matrix[ROW];
char*  keys;
char*  target;

int CTRL_PORTNO;
int CLIENT_PORTNO;
int CTRL_IN;



struct connection_info
{
	char* hostname_string;
	char* ip_address_string;
	char* service_name_str;
};

struct arg_list 
{
	int fd_sock;
	char* name_string;

};

key_t msgqkey;
int msgqid;
struct msgqstruct
{
	long mtype;
	char qbuff[MAX_BUF_SIZE];
};
#endif

