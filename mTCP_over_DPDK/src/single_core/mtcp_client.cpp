#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <iostream>
#include "mtcp_api.h"
#include "mtcp_epoll.h"
#include "dpdk_api.h"
#include "cpu.h"
#include "debug.h"
#include <time.h>

using namespace std;
int done;
mctx_t mctx;

#define MAX_EVENTS 2048
#define no_of_connections 5


void SignalHandler(int signum)
{
	//Handle ctrl+C here
	signal_handler_dpdk(signum);
	done = 1;
}

int main(int argc, char **argv)
{

	dpdkuse_ins.init_dpdkapi(argc,argv);
	char* hostname = "169.254.9.8";//argv[1];
    int portno = 9999;//atoi(argv[2]);

    char* conf_file = "main.conf";
    /* initialize mtcp */
	if (conf_file == NULL) 
	{
		TRACE_CONFIG("You forgot to pass the mTCP startup config file!\n");
		exit(EXIT_FAILURE);
	}

	//step 1. mtcp_init, mtcp_register_signal(optional)
	ret = mtcp_init(conf_file);
	if (ret) 
	{
		TRACE_CONFIG("Failed to initialize mtcp\n");
		exit(EXIT_FAILURE);
	}
	
	/* register signal handler to mtcp */
	mtcp_register_signal(SIGINT, SignalHandler);

	TRACE_INFO("Application initialization finished.\n");
	
	//step 2. mtcp_core_affinitize
	mtcp_core_affinitize(core);
	
	//step 3. mtcp_create_context. Here order of affinitization and context creation matters.
	// mtcp_epoll_create
	
	mctx = mtcp_create_context(core);
	if (!mctx) 
	{
		TRACE_ERROR("Failed to create mtcp context!\n");
		return NULL;
	}

	int ep = mtcp_epoll_create(mctx, MAX_EVENTS);
	if (ep < 0) 
	{
		TRACE_ERROR("Failed to create epoll descriptor!\n");
		return NULL;
	}
	
	//step 4. mtcp_socket, mtcp_setsock_nonblock,mtcp_bind
	int sockid = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) 
	{
		TRACE_ERROR("Failed to create listening socket!\n");
		return -1;
	}
	ret = mtcp_setsock_nonblock(mctx, sockid);
	if (ret < 0) {
		TRACE_ERROR("Failed to set socket in nonblocking mode.\n");
		return -1;
	}
	
	struct sockaddr_in saddr,daddr;
	saddr.sin_family = AF_INET;
	//saddr.sin_addr.s_addr = INADDR_ANY;
	inet_aton("169.254.9.3",&saddr.sin_addr);
	saddr.sin_port = htons(portno);

	daddr.sin_family = AF_INET;
	daddr.sin_addr.s_addr = inet_addr(hostname);
	daddr.sin_port = htons(portno);

	ret = mtcp_bind(mctx, sockid,(struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret < 0) 
	{
		TRACE_ERROR("Failed to bind to the socket!\n");
		return -1;
	}	
	cout << "Trying to connect." << endl;

	struct mtcp_epoll_event ev;
	int established_conn = no_of_connections;

	for(int i=0;i<no_of_connections;i++)
	{

		ret = mtcp_connect(mctx, sockid, (struct sockaddr *)&daddr, sizeof(struct sockaddr_in));
		if (ret < 0) 
		{
			if (errno != EINPROGRESS) 
			{
				perror("mtcp_connect");
				mtcp_close(mctx, sockid);
				return -1;
			}
		}

		ev.events = MTCP_EPOLLIN | MTCP_EPOLLOUT;
		ev.data.sockid = sockid;
		mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);

	}



	//step 6. mtcp_epoll_wait
	struct mtcp_epoll_event *events;
	int nevents;
	events = (struct mtcp_epoll_event *)calloc(MAX_EVENTS, sizeof(struct mtcp_epoll_event));
	if (!events) 
	{
		TRACE_ERROR("Failed to create event struct!\n");
		exit(-1);
	}

	char data[1448]; //1420
	uint64_t lSize=1448;

	while(1)
	{
		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, -1);
		
		if (nevents < 0) {
			if (errno != EINTR)
				perror("mtcp_epoll_wait");
			break;
		}

		for(int i=0;i<nevents;i++) 
		{
			if (events[i].events & MTCP_EPOLLIN) 
			{

					/*bzero(data,1024);
					int rd = mtcp_read(mctx, sockid, data, lSize);
					if (rd <= 0) 
					{
						return rd;
					}*/

					cout << "connection established while connect" << data << endl;
					no_of_connections--;

			}

			else if (events[i].events & MTCP_EPOLLOUT) 
			{
				int err = 0;
				socklen_t len = sizeof (int);
				cret = mtcp_getsockopt (mtcx, sockid, SOL_SOCKET, SO_ERROR, &err, &len);	//Check if connect succedd or failed??
				if( (cret != -1) && (err == 0))	//Conn estd
				{
					n = write(sockid, "done with connection", strlen(50));
					no_of_connections--;
					
				}
			}


		}

		if(no_of_connections==0)
						break;

	}

	return 0;

}

