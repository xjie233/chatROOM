#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>
#include<string.h>
#include<pthread.h>
#include<semaphore.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include"gettime.h"
#include"database.h"
#include"clientlist.h"

typedef enum
{
	false,true
}bool;

PClientlist clientlist;

int writeData(PMessage msg)
{
	if(InserteUser(msg))
	{
		printf("Insert user failed...\n");
		return 1;
	}
	return 0;
}

int alreadyExist(PMessage sourcemsg)
{
	if(IsExits(sourcemsg))
		return 1;                       //不能注册
	return 0;
}

int regCheck(PMessage msg)
{

    if(isSysword(msg->username))						//如果是关键词，注册失败
	{
		return RSGKEYWORD;
	} 
	if(alreadyExist(msg))						//如果用户名存在，注册失败
	{
		return USEREXIST;
	}
	if(writeData(msg)==0)					//注册成功
		return REGSUCCESS;
	else
		return OTHER;
}

//Login check
bool loginCheck(PMessage sourcemsg)
{
	if(LoginCheck(sourcemsg))
		return true;
	return false;
}

/*****************************/
/*服务端应该做的事情应该包括：
/*1.创建套接字->绑定端口->监听
/*2.如果有新的客户端接入，则将客户端加入到客户端队列中
/*3.处理管理员在服务器这端的信号
/*****************************/

void errClient(char *clientname)
{
	PClientlist p;
	int err;
	void *res=NULL;
	p = clientlist->next;
	printf("when call:%s\n",clientname);
	while(p)
	{
		if(strcmp(p->data->clientname,clientname)==0)		
		{
			printf("have\n");
			printf("thread %u are canceled\n",(unsigned int)(p->data->clientpid));
			err = pthread_cancel(p->data->clientpid);                          //发送终止线程信号
			if(err!=0)
			{
				printf("can't cancel thread %s \n",strerror(err));
				exit(1);
			}	
			err = pthread_join(p->data->clientpid,&res);
			if(err!=0)
			{
				printf("can't join thread %s \n",strerror(err));
				exit(1);
			}
			if(res == PTHREAD_CANCELED)
			{
				printf("thread %u has been canceled\n",(unsigned int)(p->data->clientpid));
				close(p->data->filedes);
				delClient(clientlist,clientname);                    //
				printf("%s成功退出！\n",p->data->clientname);
			}
			else
				printf("Error!\n");
			break;
		}
		p=p->next;
	}
}

//函数名：operateClient---处理客户端
//参  数：Clientnode---客户端信息结构体
//返回值：无
void operateClient(PClientnode clientnode)   //pthread_function
{
	FILE *fp;
	PMessage msg;
	char chatlog[2048];
	int current_user_num=0;
	int isLogin=0,count=0;
	Warntype warn;
	pthread_t quittid;
	char *message,*curtime,*clientname,*listuser[BACKLOG],parameter[5][TXTNUM],*tmpcur;
	int recvbytes,sendbytes=0,flag,des=0;
	PClientlist p,q;
	msg = (PMessage)malloc(sizeof(Message));                         //给指针分配内存空间
	message = (char *)malloc((TXTNUM+TIMELENGTH+20));
	tmpcur = (char *)malloc((TXTNUM+TIMELENGTH+20));
	curtime = (char *)malloc(TIMELENGTH);
	clientname = (char *)malloc(sizeof(char));
	while(1)
	{
       	if((recvbytes=recv(clientnode->filedes,msg,sizeof(Message),0))<=0)
		{
			delClient(clientlist,clientnode->clientname);
			break;                                   //这里如果有错的话，退出循环，该客户端所在线程自动退出
		}
		if(strcmp(msg->type,"regist")==0)               //注册
		{
			warn = regCheck(msg);
			//提示信息发回给客户端
			if(send(clientnode->filedes,&warn,sizeof(Warntype),0)==-1)
    			{
     				perror("operateClient(),send");
   			}
		}	
		else if(strcmp(msg->type,"login")==0)
		{
			p = clientlist->next;
			isLogin = 0;
			while(p)                     //遍历链表
			{
				if(strcmp(p->data->clientname,msg->username)==0)
				{ 
					isLogin = 1;
					break;
				}
				p=p->next;
			}
			if(isLogin)
			{ 
				warn = ALREADYLOG;
			}
			else
			{
				if(1)  //loginCheck(msg)
				{
					warn = LOGSUCCESS;
					strcpy(clientnode->clientname,msg->username);
					addClient(clientlist,clientnode);
				} 
				else
				{
					warn = USERORPWDERR;
				}
			}
			//提示信息发回给客户端
			if(send(clientnode->filedes,&warn,sizeof(Warntype),0) == -1)
    			{
     				perror("operateClient(),send");
   			}
		}
		else if(strcmp(msg->type,"all")==0)    //群聊
		{
			current_user_num=0;
			get_cur_time(curtime);
			strcpy(message,curtime);
			strcat(message,msg->username);
			strcat(message," 对所有人说： ");
			strcat(message,msg->userpass);
			strcat(message,"\n");
			//printf("message=%s\n",msg);
			fp=fopen("chatlog.txt","a");
			fwrite(message,strlen(message),1,fp);
			fclose(fp);
			p = clientlist->next;
			while(p)
			{
				current_user_num++;
				if(strcmp(p->data->clientname,msg->username)!=0)
				{
					if(send(p->data->filedes,message,strlen(message),0) == -1)
    					{
     						perror("operateClient(),send");
   				 	}
				}
				p=p->next;
			}
		}
		else if(strcmp(msg->type,"log")==0)
		{
			fp=fopen("chatlog.txt","r");
			fread(chatlog,1,strlen(chatlog),fp);
			fclose(fp);
			p = clientlist->next;
			while(p)
			{
				current_user_num++;
				if(strcmp(p->data->clientname,msg->username)==0)
				{
					if(send(p->data->filedes,message,strlen(message),0) == -1)
    					{
     						perror("operateClient(),send");
   				 	}
				}
				p=p->next;
			}
		} 
		else if(strcmp(msg->type,"personal")==0)               //私聊
		{
			des = explode(parameter,msg->userpass,'#');    //解析命令
			get_cur_time(curtime);
			strcpy(message,curtime);
			strcat(message,msg->username);
			strcat(message," 对你说：");
			strcat(message,parameter[1]);
			p=clientlist->next;
			while(p)
			{
				if(strcmp(p->data->clientname,parameter[0])==0&&strcmp(p->data->clientname,msg->username)!=0)
				{
					if(send(p->data->filedes,message,strlen(message),0) == -1)
    					{
     						perror("operateClient(),send");
   				 	}	
					break;				
				}
				p=p->next;
			}
			if(p==NULL)
			{
				if(send(clientnode->filedes,"对不起，该用户不在线\n",strlen("对不起，该用户不在线\n"),0) == -1)
    			{
     				perror("p=NULL,operateClient(),send");
   				}
			}
		} 
		else if(strcmp(msg->type,"view")==0)    //查看在线用户
		{
			memset(message,0,sizeof(message));
			p = NULL;
			p = clientlist->next;
			current_user_num = 0;
			strcpy(message,"");
			while(p)
			{
				current_user_num++;
				itoa(current_user_num,tmpcur);
				strcat(message,tmpcur);
				strcat(message,".");
				strcat(message,p->data->clientname);
				strcat(message,"\n");
				p = p->next;
			}
			if(send(clientnode->filedes,message,strlen(message),0) == -1)
    		{
     			perror("operateClient(),send");
   			}
		}
		else if(strcmp(msg->type,"trans")==0)    //传输文件
		{
			des = explode(parameter,msg->userpass,'#');
			p=clientlist->next;
			printf("%s+%s+%s",parameter[0],parameter[1],parameter[2]);
			if(strcmp(parameter[1],msg->username)==0)
			{
				if(send(p->data->filedes,"自己不能给自己发送文件!\n",strlen("自己不能给自己发送文件!\n"),0) == -1)
    			{
     					perror("operateClient(),send");
   				 }
				continue;
			}
			while(p)
			{
				if(strcmp(p->data->clientname,parameter[1])==0&&strcmp(p->data->clientname,msg->username)!=0)
				{
					if(send(p->data->filedes,message,strlen(message),0) == -1)
    				{
     					perror("operateClient(),send");
   				 	}	
					break;				
				}
				p=p->next;
			}
			if(p==NULL)
			{
				if(send(clientnode->filedes,"对不起，该用户不在线\n",strlen("对不起，该用户不在线\n"),0) == -1)
    			{
     				perror("p=NULL,operateClient(),send");
   				}
			}
		}
		else if(strcmp(msg->type,"quit")==0)
		{
			printf("server quit\n");
			p = clientlist->next;
			while(p)
			{
				delClient(clientlist,msg->username);
				break;                       //结束死循环，结束客户端在服务端的线程
			}
		} 
		else 
		{
			printf("nothing\n");
		}
		memset(msg,0,sizeof(Message));
	}
}

//函数名：acceptClient---接受客户端连接
//参数：serverfd---服务端创建的套接字
//返回值：无
void acceptClient(int *serverfd)            //pthread_function
{

	int sockfd = *serverfd,addr_length;
	PClientnode clientnode;
	pthread_t pid;
	struct sockaddr_in clientaddr;
	addr_length = sizeof(struct sockaddr_in);


	while(1)
	{
		clientnode = (PClientnode)malloc(sizeof(Clientnode));
		if((clientnode->filedes = accept(sockfd,(struct sockaddr*)&clientaddr,&addr_length))==-1) //accept返回的是与客户端建立的新的套接字文件描述符，之后的通信就是通过这个描述符
		{
			perror("acceptClient,accept");
			exit(1);
		}
	
		printf("accept from :%d\n",(int)(ntohs(clientaddr.sin_port)));   //将一个无符号短整形数从网络字节顺序转换为主机字节顺序。
		clientnode->clientaddr_in = clientaddr;
		pthread_create(&clientnode->clientpid,NULL,(void *)operateClient,(void *)clientnode);
	}
}

//函数名：listclient－－－列出当前在线的用户
//参数：无
//返回值：无
void listClient()
{
	PClientlist p;
	int current_client_num=0;
	p = clientlist->next;
	printf("当前在线用户为：\n");
	disClist(clientlist);
}
//函数名：quitClient---踢出某用户
//参数：clientname---用户名
//返回值:0－踢出失败，1-踢出成功
void quitClient()
{
	int err;
	void *res=NULL;
	char clientname[20];
	PClientlist p;	
	printf("请输入你要踢出的客户端的名称：");
	scanf("%s",clientname);
	p = clientlist->next;
	while(p)
	{
		if(strcmp(p->data->clientname,clientname)==0)
		{				
				err = pthread_cancel(p->data->clientpid);
				if(err!=0)
				{
					printf("can't cancel thread %s \n",strerror(err));
					exit(1);
				}	
				err = pthread_join(p->data->clientpid,&res);
				if(err!=0)
				{
					printf("can't join thread %s \n",strerror(err));
					exit(1);
				}
				if(res == PTHREAD_CANCELED)
				{
					printf("踢出%s成功！\n",p->data->clientname);
					if(send(p->data->filedes,"你已经被强制下线！\n",strlen("你已经被强制下线！\n"),0) == -1)
    				{
     					perror("p=NULL,operateClient(),send");
   					}
					close(p->data->filedes);
					delClient(clientlist,clientname);
				}
				else
					printf("Error!\n");	
				close(p->data->filedes);
				break;
		}
		p = p->next;
	}	
	if(p==NULL)
	{
		printf("该用户不在线！\n");
	}
}
//函数名：closeServer---关闭服务器
//参数：无
//返回值：无
void closeServer()
{
	destoryClist(&clientlist);	
	exit(1);
}

//函数名：handServer---处理客户端函数
//参数：无
//返回值：无
void handleServer(void)
{
	int choice;
	while(1)
	{
		printf("**************************\n");
		printf("****1.查看当前在线用户********\n");
		printf("****2.踢出某客户端************\n");
		printf("****3.关闭服务器**************\n");
		printf("**************************\n");
		scanf("%d",&choice);
		while(choice>3||choice<1)
		{
			printf("你输入的数字有误，请重新输入：\n");
			scanf("%d",&choice);
		}
		switch(choice)
		{
			case 1:listClient();break;
			case 2:quitClient();break;
			case 3:closeServer();break;
		}
	}
}

//端口绑定函数,创建套接字，并绑定到指定端口
int bindPort(unsigned short int port)
{  
   int sockfd,clientfd,sin_size,recvbytes;
   struct sockaddr_in my_addr;
   sockfd = socket(AF_INET,SOCK_STREAM,0);			//创建基于流套接字TCP
   my_addr.sin_family = AF_INET;					//IPv4协议族
   my_addr.sin_port = htons(port);					//端口转换
   my_addr.sin_addr.s_addr = INADDR_ANY;			//服务器可以接受任意地址
   bzero(&(my_addr.sin_zero),0);

   if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr)) == -1)
   {
     perror("bind:");
     exit(1);
   }
   printf("bing success!\n");
   return sockfd;
}

int main(int argc, char *argv[])
{
        int sockfd,res; 								//定义监听套接字、客户套接字
 	struct sockaddr_in their_addr;  					//定义地址结构
        pthread_t pid;								//线程ID
        char *buf,*str;								//定义缓冲区
        sockfd  = bindPort(MYPORT);					//绑定定义缓冲区端口
	clientlist = createList();
    if(listen(sockfd,BACKLOG) == -1)					//在指定端口3492上监听
    {
       perror("listen");
       exit(1);
    }
    printf("正在监听......\n");
	//此创建一个线程用来监听新加入的客户端
	res = pthread_create(&pid,NULL,(void *)acceptClient,(void *)&sockfd);
	//此处用来处理服务器端的操作
	handleServer();
	return 0;	
}
