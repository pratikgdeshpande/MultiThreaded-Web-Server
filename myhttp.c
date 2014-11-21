#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>
#include<fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#define BUF_SIZE 1024
sem_t lsem;
sem_t sem;+
sem_t wth;
sem_t cth;
void* work(void* msg);
void display();
void* threadp(void* somthing);
void printUsage(void);
int q_empty;
int debug = 0;
int port = 8080;
char *logfile = NULL;
char *rootdir = "/";
int root=0;
struct request del();
void *list();



//*********QUEUE MECHANISMS*************
struct request
{
	char fname[100];
	char req[100];
	int get;//TO CHECK WHETHER REQUEST HEAD/GET OR OTHER
	int csockfd;//socketfd of client	
	long long size;//suppose to give size
	char ip[100];//ip address
	char time[100];//TIME IT WAS ADDED TO THE QUING
	char stime[100];// time the request is schedduled
	int status;//200 for file found 404 for file not found
	char direc[100];//for keeping the current directory
}rq;


struct node
	{
		struct request r;
		struct node* next;
	}*front=NULL,*rear=NULL;


struct threadpool
{
	int numthrds;//TO KEEP TRACK OF NUMBER OF THREADS
        pthread_t* wthreads;//POINTER TO WORKER THREADS
}thpool;

/*function to sort the request queue in shortest job first format*/
void sort()
{	
	struct node* temp3=malloc(sizeof(struct node));
	struct node* temp4=malloc(sizeof(struct node));
	temp3=front;
	temp4=temp3->next;
	struct request temp5;
	if(temp3==NULL)
	{}
	else if(temp4==NULL)
	{
	//printf("only one request %lld",temp3->r.size);
		
	}
	else
	{
		while(temp3->next!=NULL)
    			{
       				while(temp4!=NULL)
        			{
            				if((temp3->r.size)>(temp4->r.size))
           				 {
						temp5=temp4->r;
						temp4->r=temp3->r;
						temp3->r=temp5;
                
            				  }
          			temp4=temp4->next;
        			}
		      	temp3=temp3->next;
		        temp4=temp3->next;
			}

	}
	
	//display();
}

/*function to extract a request from the request queue*/
struct request del()
{ 
	
	struct request ret;//TO RETURN THE EXTRACTED REQUEST 
	struct node* temp2=malloc(sizeof(struct node));
	if(front==NULL)
	{//printf("q empty");
}
	else
	{
	ret=front->r;
	front=front->next;
	}
	//printf("\ndeleted item%s",ret.fname);
	free(temp2);
	return ret;

}

/*a debugging function to check what reuqests are currently in queue*/
void display()
{ 
	struct node* temp=malloc(sizeof(struct node));
	temp=front;
	//printf("\n");
	while(temp!=NULL)
	{

		//printf("[filename-%s]--->",temp->r.fname);
		temp=temp->next;
	}
	//if(rear==NULL&&front==NULL)
	//printf("\nq empty");
	
	

}

/*function to add request in queue*/
void addq(struct request r1)
{	
	struct node* temp1=malloc(sizeof(struct node));
	temp1->r=r1;
	
	   
	//printf("\n added file-%s",temp1->r.fname);
	
	if(front==NULL)
	{
	 front=temp1;
	}

	else if(rear==NULL)
	{
	 front->next=temp1;
	 rear=temp1;
	}
	else
	{
	 rear->next=temp1;
	 rear=temp1;
	}
	
       
	//display();
 

}


//*********END OF QUEUE MECHANISMS*************


/*function to log requests*/
void logg(struct request req)
{

	char size[100];
	char *filename = req.fname;
	struct stat buf;
	int ret = stat((const char*) filename, &buf);
	long long num=(long long) buf.st_size;
	//snprintf(size,sizeof(size),"%lld",num);

	//printing on server 
	if(debug!=0)
	{
	printf("%s",req.ip);
	printf("-[%s]",req.time);
	printf("[%s]",req.stime);
	printf("  %s",req.req);
	snprintf(size,sizeof(size),"%d",req.status);
	printf("   %s",size);
	snprintf(size,sizeof(size),"%lld",num);
	printf(" %s",size);
	}
	
	
	if(logfile!=NULL)
	{FILE *f = fopen(logfile, "a+");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	//error in writing file 
	fprintf(f,"%s",req.ip);
	fprintf(f, "[%s]",req.time);
	fprintf(f, " [%s]",req.stime);
	fprintf(f, "   %s",req.req);
	fprintf(f, "   %d",req.status);
	fprintf(f, "   %s\n",size);
	fclose(f);
	}
}


/*function to provide head comman information*/
void head(struct request rec)
{
	int sock=rec.csockfd;

	char request[100];
	memset(request, 0,100);
	char *filename = rec.fname;
	int n, ret = 0;
	char ctype[10];
	memset(ctype, 0,10);
	struct stat buf;
	ret = stat((const char*) filename, &buf);
	time_t now = time(NULL);
	strcpy(request,"\nDATE\t\t:");
	write(sock,request,strlen(request));
	strcpy(request,asctime( gmtime((const time_t*)&now) ));
	write(sock,request,strlen(request));
	strcpy(request,"SERVER\t\t:SERVER 21.2");
	write(sock,request,strlen(request));
	strcpy(request,"\nLast-Modified\t:");
	write(sock,request,strlen(request));
	if(ret==0)
	strcpy(request,asctime(gmtime(&(buf.st_mtime))));
	else
	strcpy(request,"FNF");
	write(sock,request,strlen(request));
	//checking content type
	if(strstr(rec.fname,".txt"))
	strcpy(ctype,"text");
	if(strstr(rec.fname,".html"))
	strcpy(ctype,"html");
	if(strstr(rec.fname,".jpg")||strstr(rec.fname,".bmp"))
	strcpy(ctype,"image");
	if(strstr(rec.fname,".gif"))
	strcpy(ctype,"gif");
	if(strstr(rec.fname,".c"))
	strcpy(ctype,".c");
	if(strstr(rec.fname,".cpp"))
	strcpy(ctype,".cpp");
	//done with content check
	strcpy(request,"\nContent Type \t:");
	write(sock,request,strlen(request));
	strcpy(request,ctype);
	write(sock,request,strlen(request));
	strcpy(request,"\nContent Length\t:");
	write(sock,request,strlen(request));
	memset(request, 0,100); 
	int i=0;
	long long num=(long long) buf.st_size;
	snprintf(request,sizeof(request),"%lld bytes",num);
	write(sock,request,strlen(request));
	write(sock,"\n",2);
	write(sock,"  ",3);
	memset(request, 0,100);



}



/*function to read a file*/
void rd_file(struct request rec)
{
	int sock=rec.csockfd;
	struct stat buf;
	
	 
	
	char *filename =rec.fname;
	FILE* fp1;
	int ret,n;
	size_t read;
	
	struct stat filestat;
	ret = stat( filename, &filestat );
	long long fsize=filestat.st_size;
	char* readBuf=NULL;
	readBuf = (char*) malloc( sizeof(char) * 1025);
	bzero(readBuf,1025);
	//printf("\n inside readfile");
	if(!(strstr(rec.fname,"FNF")))
 	{
		rec.status=200;//file found
		fp1 = fopen( filename, "r" );
	
		if (fp1 == NULL) {
		fprintf(stderr, "File (%s) not found, or other problem\n", filename);
		}
	 	//printf("problem in while");
		while(fsize>0)
		{
			if(fsize>1024)
				{
			
				n=fread(readBuf,1,1024,fp1 );//read file in chunks
				//printf("\nentered loop1");
				n=send(sock,readBuf,1024,0);//send file in chunks
				fsize=fsize-1024;
				}
			else
				{
				//printf("\nentered loop1");
				n=fread(readBuf,1,fsize, fp1 );
				n=send(sock,readBuf,fsize,0);
				readBuf[n]='\0';
				fsize=0;
				//printf("\n%s",readBuf);
				}

		}
	}

	else
		{	int i,j;
			char dir_files[1024][256];
			rec.status=404; //file not found
			DIR   *directory;
		  	struct dirent *dir;
			directory =opendir(rq.direc);
		  	if (directory)
                        {
                            strcat(readBuf,"Directory contents :\n");
                            i=0;
                                while ((dir = readdir(directory)) != NULL)
                                {
                                if(dir->d_name[0] != '.')
                                {
                                    strcpy(dir_files[i],dir->d_name);
                                    i++;
                                }
                               
                                }
                            int n=i;
                            i=0;
                            char temp[25];
                            for(i=0;i<=n;i++)
                            {
                                for(j=0;j<=n;j++)
                                {
                                    if(strcmp(dir_files[i],dir_files[j]) < 0) ///sorting directory
                                    {
                                      strcpy(temp,dir_files[i]);
                                    strcpy(dir_files[i],dir_files[j]);   
                                    strcpy(dir_files[j],temp);
                                    }
                                }
                            }
                            for(i=0;i<=n;i++)
                            {
                                strcat(readBuf,dir_files[i]);
                                strcat(readBuf,"\n");
                            }
                            send(sock,readBuf,strlen(readBuf),0);
                            closedir(directory);
                          }
		     
	}			
	close(sock);
	free(readBuf);

}

/*thread pool initializer*/
void init_thrdpool(int n)
{
	struct threadpool* tp;
	tp=(struct threadpool*)malloc(sizeof(struct threadpool));
	tp=(&thpool);
	tp->numthrds=n;
	tp->wthreads=(pthread_t*) malloc (sizeof(pthread_t) * n);
int i;
	  for (i = 0;i < n;i++) {
		  if(pthread_create(&(tp->wthreads[i]),NULL,work,tp)) {
		    //fprintf(stderr, "Thread initiation error!\n");
			pthread_join(tp->wthreads[i],NULL);	
					
		  }

	  }
 
	//printf("i reached here"); 

}

/*master thread function*/
void *mstthread(void* sche)
{	
	//printf("\ninside scheduling function");
	int *temp=(int*)sche;
	int sc=*temp;
	while(1)
	{
	if(!q_empty){	
			sem_wait(&lsem);
			{
				sem_wait(&sem);
			
				
			
				{
				 sem_wait(&wth);
		
				if(sc)
				{
					//printf("\n applying sjf");
					sort();
				}
	
				rq=del();
				//printf("\n file extracted");
				//printf("\nqlock removed");
				sem_post(&cth);
				
				}
					
			}
	

			
		
		
	
		}
	else
	{
		continue;
	}
}

	
}


/*worker thread function*/
void* work(void* msg)
{	

	time_t now = time(NULL);
	sem_wait(&cth);
	
	struct request serq;//serving request 
	          
        
        serq=rq;
	sem_post(&wth);
	
         strcpy(serq.stime,strtok(asctime( gmtime((const time_t*)&now) ),"\n"));
	if(front==NULL)
        q_empty=1;
	
	
	if(serq.get==1)
	{
	head(serq);	//if get then print both file and metadata
	rd_file(serq);
	}
	else if (serq.get==2)   //if head then only meta data
	{head(serq);}	//only head

	logg(serq);
	
	
	sem_post(&sem);
	

}

/*function to parse the request*/
void parse(char *req)
{
		struct stat buf;
		char request[256];
		char temp[256];
		strcpy(request,req);
		strcpy(rq.req,request);
		const char token1[2]="/";
		const char token2[2]=" ";
		const char token3[4]="%20";
		//to check which request this is GET or HEAD
		char *parsed=strtok(request,token1);
		//printf("\ncommand=[%s]",parsed);
		
		if(strstr(parsed,"GET")||strstr(parsed,"GET "))	
		rq.get=1;
		else if(strstr(parsed,"HEAD")||strstr(parsed,"HEAD "))
		rq.get=2;
		else
		rq.get=3;
		//done with checking type of request
		//to find file name
		parsed = strtok(NULL, token2);//to remove the sapce after file name 
		strcpy(rq.fname,parsed);
		strcpy(rq.direc,".");
		if(strstr(rq.req,"~"))		
		{	
		char user[29]="/home/";
		strcat(user,getenv("USER"));
		strcat(user,"/myhttpd/");
		int i,j=0;
		for(i=1;i<strlen(rq.fname);i++)
		{
		
		temp[j]=rq.fname[i];//removing the ~
		j++;
		}
		strcpy(rq.direc,user);
		strcat(user,temp);
		memset(rq.fname,0,256);// making rqfname zero
		strcpy(rq.fname,user);
		}
		
		
		
		
	        
}

/*function to listen to incoming requests*/
void *list(void* prtno)
{
	struct stat buf;
	time_t now = time(NULL);
	char requ[256];
	int *p=(int*)prtno;
	int sockfd, newsockfd, portno, clilen,ret;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	 char temp[100];  
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	{error("ERROR opening socket");}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno =*p; 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
	{error("ERROR on binding");}
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	if(root==1)
	{
	chdir(rootdir);
	}//change the root directory
	while(1)
	{
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
/////////////////////////////IP ADDRESS///////////////////////////////////////////////////////////////

	unsigned char *IPAddress;
	char str[10];
	IPAddress = (unsigned char *)&cli_addr.sin_addr.s_addr;
	sprintf(str,"%d",IPAddress[0]);
	strcpy(rq.ip,str);
	strcat(rq.ip,".");
	sprintf(str,"%d",IPAddress[1]);
	strcat(rq.ip,str);
	strcat(rq.ip,".");
	sprintf(str,"%d",IPAddress[2]);
	strcat(rq.ip,str);
	strcat(rq.ip,".");
	sprintf(str,"%d",IPAddress[3]);
	strcat(rq.ip,str);
	
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (newsockfd < 0) 
		error("ERROR on accept");
		
		bzero(buffer,256);
		n = read(newsockfd,buffer,255);
		strcpy(requ,buffer);
		bzero(buffer,256);
		rq.csockfd=newsockfd;//adding clients sockfd to request
/////////////////ALL THE PARSING WORK//////////////////////////
		parse(requ);
		char c='/';
		if(rq.fname[strlen(rq.fname)-1]==c)  //if it is a directory
		{
		//memset(direc,0,100);
		strcpy(rq.direc,rq.fname);//copying the directory location
		strcat(rq.fname,"index.html");
		}
		ret = stat((const char*) rq.fname, &buf);
		if(!(ret==0))
		strcpy(rq.fname,"FNF");//if file not present
////////////////check if file present///////////////////		
		if(!(strcmp("FNF",rq.fname)))
		rq.status=404;//file not found
		else
		rq.status=200;//file found
/////////////////////////////////////////////////////////////
		rq.size=(long long) buf.st_size;
		strcpy(rq.time,strtok(asctime( gmtime((const time_t*)&now) ),"\n"));
		if(strstr("~",rq.req))
{
		if(strstr("GET",rq.req)||strstr("GET ",rq.req))	
		rq.get=1;
		else if(strstr("HEAD",rq.req)||strstr("HEAD ",rq.req))
		rq.get=2;
		else
		rq.get=3;
		rq.csockfd=newsockfd;
		
}
		//printf("\nrequest-%s\tfilename-%s\tsize-%lld\ttime-%s\trequest type=%d\tclient socket=%d",rq.req,rq.fname,rq.size,rq.time,rq.get,rq.csockfd);
		addq(rq);
		sem_post(&lsem);
	     
	}
}





int main(int argc, char *argv[])
{

	
	int n_threads = 4;
	int queueing_time = 60;
	 
	char *schedPolicy = "FCFS";
	 
	int c,policy;
	 
	opterr = 0;
	 
	 
	// You should give a list of options in the third argument to
	// getopt() ":" means you want to get a value after the option.
	while ( ( c = getopt (argc, argv, "dhl:p:r:t:n:s:") ) != -1 )
	{
		switch (c)
		{
		case 'd':
		debug = 1;
		break;
		case 'h':
		printUsage();
		exit(1);
		case 'l':
		logfile = optarg;
		break;
		case 'p':
		port = atoi(optarg);
		if (port < 1024)
		{
		fprintf(stderr, "[error] Port number must be greater than or equal to 1024.\n");
		exit(1);
		}
		break;
		case 'r':
		{rootdir = optarg;
		root=1;}
		break;
		case 't':
		queueing_time = atoi(optarg);
		if (queueing_time < 1)
		{
		fprintf(stderr, "[error] queueing time must be greater than 0.\n");
		exit(1);
		}
		break;
		case 'n':
		n_threads = atoi(optarg);
		if (n_threads < 1)
		{
		fprintf(stderr, "[error] number of threads must be greater than 0.\n");
		exit(1);
		}
		break;
		case 's':
		schedPolicy = optarg;
		break;
		default:
		printUsage();
		exit(1);
		}
	} // while (...)
	 
	if (debug == 1)
	{
	fprintf(stderr, "myhttpd logfile: %s\n", logfile);
	fprintf(stderr, "myhttpd port number: %d\n", port);
	fprintf(stderr, "myhttpd rootdir: %s\n", rootdir);
	fprintf(stderr, "myhttpd queueing time: %d\n", queueing_time);
	fprintf(stderr, "myhttpd number of threads: %d\n", n_threads);
	fprintf(stderr, "myhttpd scheduling policy: %s\n", schedPolicy);
	}
	 

 	if(strstr("SJF",schedPolicy))
	{
		policy=1;
	}
	else
	policy=0;
	pthread_t qing,schd;
	sem_init(&sem,0,1);
	sem_init(&lsem,0,0);
	sem_init(&cth,0,0);
	sem_init(&wth,0,1);
	int r1,r2,no,temp;
	temp=0;
	int *sjf,*pno;
	if(debug!=1)
	{daemon(1,0);}
	pno=&port;
	sjf=&policy;
	if(debug)
	{n_threads=1;}
	init_thrdpool(n_threads);//initializing number of threads
	pthread_create(&qing,NULL,list,(void*)pno);
	sleep(queueing_time);
	pthread_create(&schd,NULL,mstthread,(void*) sjf);
	pthread_join(qing,NULL);
	pthread_join(schd,NULL);
        
        


	return 0;
	}

/*help function*/
	void printUsage(void)
	{
	fprintf(stderr, "Usage: myhttpd [âˆ’d] [âˆ’h] [âˆ’l file] [âˆ’p port] [âˆ’r dir] [âˆ’t time] [âˆ’n thread_num] [âˆ’s sched]\n");
	 
	fprintf(stderr,
	"\tâˆ’d : Enter debugging mode. That is, do not daemonize, only accept\n"
	"\tone connection at a time and enable logging to stdout. Without\n"
	"\tthis option, the web server should run as a daemon process in the\n"
	"\tbackground.\n"
	"\tâˆ’h : Print a usage summary with all options and exit.\n"
	"\tâˆ’l file : Log all requests to the given file. See LOGGING for\n"
	"\tdetails.\n"
	"\tâˆ’p port : Listen on the given port. If not provided, myhttpd will\n"
	"\tlisten on port 8080.\n"
	"\tâˆ’r dir : Set the root directory for the http server to dir.\n"
	"\tâˆ’t time : Set the queuing time to time seconds. The default should\n"
	"\tbe 60 seconds.\n"
	"\tâˆ’n thread_num : Set number of threads waiting ready in the execution thread pool to\n"
	"\tthreadnum. The d efault should be 4 execution threads.\n"
	"\tâˆ’s sched : Set the scheduling policy. It can be either FCFS or SJF.\n"
	"\tThe default will be FCFS.\n"
	);
}


