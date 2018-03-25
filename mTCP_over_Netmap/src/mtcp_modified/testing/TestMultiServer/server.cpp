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
#include <pthread.h>
#include <signal.h>
#include <iostream>
#include <mtcp_api.h>
#include <mtcp_epoll.h>
#include <strings.h>
#include <vector>
#include "cpu.h"
#include "debug.h"
#include "netmap_api.h"

#define MAX_EVENTS 1024
#define MAX_THREADS 4
using namespace std;
/*----------------------------------------------------------------------------*/

struct arg{
	int id;
	int coreno;
};

pthread_t servers[MAX_THREADS];
struct arg arguments[MAX_THREADS];
int done[MAX_THREADS];
int portno;

/*----------------------------------------------------------------------------*/

vector < compute_stats > stats;
struct timeval begin_t;
struct timeval end_t;
bool safe_to_compute_tput = false;
uint64_t total;
double rate_t, diff_t;

/*----------------------------------------------------------------------------*/

void 
CloseConnection(mctx_t mctx, int ep, int sockid)
{
	mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_DEL, sockid, NULL);
	mtcp_close(mctx, sockid);
}


void app_level_stats() {

	gettimeofday(&end_t, NULL);
	diff_t = (end_t.tv_sec - begin_t.tv_sec) + 
              ((end_t.tv_usec - begin_t.tv_usec)/1000000.0);
	
	for(int i=0;i<MAX_THREADS;i++){
		total+= stats[i].forwarded;
	}
	
	rate_t = (total*8) / (diff_t*1024*1024*1024);
	D("Rate in Gbps: %lf", rate_t);
	
}

void
SignalHandler(int signum)
{
	//Handle ctrl+C here
	netmapuse.do_abort = 1;
	for(int i = 0;i<MAX_THREADS;i++){
		done[i] = 1;	
	}
	app_level_stats();
}

void serverThreadFunc(void* arg1){
	
	struct arg argument = *((struct arg*)arg1);
	int core = argument.coreno; 
	int id = argument.id;
	//step 2. mtcp_core_affinitize
	mtcp_core_affinitize(core);
	
	//step 3. mtcp_create_context. Here order of affinitization and context creation matters.
	// mtcp_epoll_create
	
	mctx_t mctx = mtcp_create_context(core);
	if (!mctx) {
		TRACE_ERROR("Failed to create mtcp context!\n");
		return NULL;
	}
	else{
		TRACE_INFO("mtcp context created.\n");
	}
	
	sleep(10);
	/* create epoll descriptor */
	int ep = mtcp_epoll_create(mctx, MAX_EVENTS);
	if (ep < 0) {
		TRACE_ERROR("Failed to create epoll descriptor!\n");
		return NULL;
	}
	
	//step 4. mtcp_socket, mtcp_setsock_nonblock,mtcp_bind
	int listener = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		TRACE_ERROR("Failed to create listening socket!\n");
		return;
	}
	int ret = mtcp_setsock_nonblock(mctx, listener);
	if (ret < 0) {
		TRACE_ERROR("Failed to set socket in nonblocking mode.\n");
		return;
	}
	
	struct sockaddr_in saddr;
	
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(portno);
	
	ret = mtcp_bind(mctx, listener,(struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		TRACE_ERROR("Failed to bind to the listening socket!\n");
		return;
	}

	//step 5. mtcp_listen, mtcp_epoll_ctl
	/* listen (backlog: 4K) */
	ret = mtcp_listen(mctx, listener, 4096);
	if (ret < 0) {
		TRACE_ERROR("mtcp_listen() failed!\n");
		return;
	}
		
	/* wait for incoming accept events */
	struct mtcp_epoll_event ev;
	ev.events = MTCP_EPOLLIN;
	ev.data.sockid = listener;
	char data[DATA_LEN];
	mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, listener, &ev);
	
	//step 6. mtcp_epoll_wait
	struct mtcp_epoll_event *events;
	int nevents;
	int newsockfd = -1;
	events = (struct mtcp_epoll_event *)calloc(MAX_EVENTS, sizeof(struct mtcp_epoll_event));
	if (!events) {
		TRACE_ERROR("Failed to create event struct!\n");
		exit(-1);
	}
	
	while(!done[core]){
		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, -1);
		
		if (nevents < 0) {
			if (errno != EINTR)
				perror("mtcp_epoll_wait");
			break;
		}
		
		for(int i=0;i<nevents;i++) {
			if (events[i].data.sockid == listener) {
				//Accept connection
				newsockfd = mtcp_accept(mctx, listener, NULL, NULL);
				ev.events = MTCP_EPOLLIN;
				ev.data.sockid = newsockfd;
				mtcp_setsock_nonblock(mctx, newsockfd);
				mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, newsockfd, &ev);
				D("New connection at core %d", core);
			}
			else if (events[i].events & MTCP_EPOLLIN) {
				ret = mtcp_read(mctx, newsockfd, data, DATA_LEN);
				if (ret <= 0) {
					CloseConnection(mctx,ep,newsockfd);
					return NULL;
				}
				if(core == 0 || core == 2)
					D("%s",data);
				if(safe_to_compute_tput) {
					stats[core].forwarded += ret;
				}
			}	
		}
	}
	
	return NULL;
}

int main(int argc, char **argv){
	
	int core = 0;
	int ret = -1;
	
	if (argc != 2) {
       fprintf(stderr,"usage: <port>\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[1]);
    
    char* conf_file = "server.conf";
    /* initialize mtcp */
	if (conf_file == NULL) {
		TRACE_CONFIG("You forgot to pass the mTCP startup config file!\n");
		exit(EXIT_FAILURE);
	}
	else {
		TRACE_INFO("Reading configuration from %s\n",conf_file);
	}
	//step 1. mtcp_init, mtcp_register_signal(optional)
	ret = mtcp_init(conf_file);
	if (ret) {
		TRACE_CONFIG("Failed to initialize mtcp\n");
		exit(EXIT_FAILURE);
	}
	
	/* register signal handler to mtcp */
	mtcp_register_signal(SIGINT, SignalHandler);

	TRACE_INFO("Application initialization finished.\n");
	for(int i=0;i<MAX_THREADS;i++){
		done[i] = 0;
	}
	
	stats.resize(MAX_THREADS);
	
	for(int i=0;i<MAX_THREADS;i++){
		memset(&stats[i], 0, sizeof(struct compute_stats));
	}
	//spawn server threads
	for(int i=0;i<MAX_THREADS;i++){
		arguments[i].coreno = i;
		arguments[i].id = i;
		pthread_create(&servers[i],NULL,serverThreadFunc,&arguments[i]);
	}
	
	sleep(60);
	
	safe_to_compute_tput = true;
	gettimeofday(&begin_t, NULL);
	
	//Wait for server threads to complete
	for(int i=0;i<MAX_THREADS;i++){
		pthread_join(servers[i],NULL);		
	}
	
	return 0;
}
