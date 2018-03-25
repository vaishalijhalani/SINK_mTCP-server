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
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

#include <set>
#include <iterator>
#include <map>

#define MAX_EVENTS 2048
#define no_of_connections 5

using namespace std;
int done;

void setsock_nonblock(int fd)
{
    int flags;
   
    flags = fcntl(fd, F_GETFL, NULL);

    if(-1 == flags)
    {
        printf("fcntl F_GETFL failed.%s", strerror(errno));
        exit(1);
    }

    flags |= O_NONBLOCK;

    if(-1 == fcntl(fd, F_SETFL, flags))   
    {
        printf("fcntl F_SETFL failed.%s", strerror(errno));
        exit(1);
    }       
}


int main(int argc, char **argv){

  float packets=0, sec=0;
  clock_t before;

  float rate;
  
  int core = 0;
  int ret = -1;
  done = 0;

  
    char* hostname = "10.129.131.185";//argv[1];
    int portno = 9999;//atoi(argv[2]);
  
  /* create epoll descriptor */
  int ep = epoll_create( MAX_EVENTS);
  if (ep < 0) {
    printf("Failed to create epoll descriptor!\n");
    return NULL;
  }
  
  //step 4. socket, setsock_nonblock,bind
  int listener = socket( AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    printf("Failed to create listening socket!\n");
    return -1;

  }

  int optval =1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

  
  
  struct sockaddr_in saddr;
  
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr("10.129.131.185");
  saddr.sin_port = htons(portno);
  
  ret = bind( listener,(struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
  if (ret < 0) {
    printf("Failed to bind to the listening socket!\n");
    return -1;
  }

  setsock_nonblock( listener);

  //step 5. listen, epoll_ctl
  /* listen (backlog: 4K) */
  ret = listen( listener, 4096);
  if (ret < 0) {
    printf("listen() failed!\n");
    return -1;
  }
    
  /* wait for incoming accept events */
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = listener;
  epoll_ctl( ep, EPOLL_CTL_ADD, listener, &ev);
  
  //step 6. epoll_wait
  
  
  cout << "Waiting for events in server" << endl;

  
  //step 6. epoll_wait
  struct epoll_event *events;
  int nevents;
  events = (struct epoll_event *)calloc(MAX_EVENTS, sizeof(struct epoll_event));
  if (!events) {
    printf("Failed to create event struct!\n");
    exit(-1);
  }
  int newsockfd = -1;
  char data[1448]; //1420
  uint64_t lSize=1448;
  set<int> list1;
  
  int x = no_of_connections;
   int k=0;
  while(1)
  {
  	
    nevents = epoll_wait( ep, events, MAX_EVENTS, 100);
 
    if (nevents < 0) 
    {
      if (errno != EINTR)
        printf("epoll_wait");
      break;
    }

    //cout << "nevent  " << nevents;
    for(int i=0;i<nevents;i++) 
    {


     // cout << "checking if else in for loop";
      if (events[i].data.fd == listener) 
      {
      	//cout <<"value of k in if " << k;
        newsockfd = accept( listener, NULL, NULL);
        if(newsockfd < 0)
        {
						
						if((errno == EAGAIN) ||	//Need lsfd non blocking to run this!!!!!!
						   (errno == EWOULDBLOCK))
						{
							cout << "processed all connections !!!" << endl;
							break;
						}
						else
						{
							cout<<"Error on accept"<<'\n';
							break;
						}
						//exit(-1);
		}

        printf("New connection accepted.\n");
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = newsockfd;
        setsock_nonblock( newsockfd);
        list1.insert(newsockfd);
        int retval = epoll_ctl(ep, EPOLL_CTL_ADD, newsockfd, &ev);
        if( retval == -1)
        {
   						cout<<"Error: epoll ctl lsfd add"<<'\n';
   						exit(-1);
   		}
     
      }

      else if (list1.find(events[i].data.fd) != list1.end())
      {

      	if(events[i].events & EPOLLIN)
      	{
      
	      		//cout <<"value of k in if else " << k;
	      	  //epoll_ctl(ep, EPOLL_CTL_DEL, events[i].data.fd, &ev);
	          int ret1 = read(events[i].data.fd, data, lSize);
	          if (ret1 < 0) 
	          {
	            printf("Connection closed with client.\n");
	      		break;
	          }
	          //cout << "data came from client " << data << endl;


	          int recv_num = atoi(data);
	          recv_num *= 100;
	          //cout << recv_num << endl;
	          char buf[100];
	          sprintf(buf,"%d", recv_num);
	          //cout << "int in buffer " << buf;
	          int n = write(events[i].data.fd, buf, 100);
	          if(n < 0)
	           cout << "data is not written " << n;


   		}


          

      }
    }

   // k++;

    //sleep(1);
    
  }

 
  return 0;
}
