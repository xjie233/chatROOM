#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netinet/in.h>

typedef struct clientinfo
{
        char	 clientname[30];		              //客户端用户名
        struct	 sockaddr_in clientaddr_in;		//客户端socket地址
        pthread_t 	clientpid;		                //客户端所在的线程ID
        int	filedes;		                        //套接字描述符
}*PClientnode, Clientnode;

typedef PClientnode DataType;				//定义数据类型


//define linlist
typedef struct linklist
{
        DataType		data;				//节点的成员数据
	struct linklist * 	next;				//指向下一个节点的指针
}Clientlist,*PClientlist;

PClientlist createList(void)
{
	PClientlist H;
	H = (PClientlist) malloc(sizeof(Clientlist));
	if(H)
		H->next=NULL;
	return H;
}
//显示整个链表
void disClist(PClientlist  H)	
{
	PClientlist p;
	int current_client_num=0;
	p = H->next;
	if(p==NULL) 
	{
		printf("无用户在线！\n");
		return ;
	}
	while(p)
	{
		current_client_num++;
		printf("%d.%s\n",current_client_num,p->data->clientname);
		p = p->next;
	}
}
//删除整个链表
void destoryClist(PClientlist  *H)			
{
//至于这里为什么要传指针的指针？
//我的理解：如果我们传H的话，在销毁时并不能控制H指向
//传的是*H的话，可以在最后令	*H=NULL，表示让H不知想任何内存地址

	PClientlist p,q;
	p = *H;
	while(p)
	{
		q=p;
		p=p->next;
		if(p==NULL) continue;
		if(send(p->data->filedes,"对不起，服务器已经关闭！\n",strlen("对不起，服务器已经关闭！\n"),0)==-1)
    	{
     		perror("destory,send");
   		}		
		free(q);
	}
	*H=NULL;
}
//向链表中增加元素
void addClient(PClientlist H,PClientnode node)
{
	PClientlist p,cnode;
	cnode = (PClientlist)malloc(sizeof(Clientlist));
	cnode->data = node;
	cnode->next = NULL;
	p = H;
	while(p->next)
	{
		p=p->next;
	}
	p->next = cnode;
}
//查找链表中元素
PClientlist serClient(PClientlist H,char * clientname)
{
	PClientlist p;
	p = H->next;
	while(p)
	{
		if(strcmp(p->data->clientname,clientname)==0)
		{return p;}
		p=p->next;
	}
	return NULL;
}
//根据客户端名称删除链表中元素
void delClient(PClientlist H,char clientname[20])
{
	PClientlist p,q;
	p = H->next;
	q = H;
	while(p)
	{
		if(strcmp(p->data->clientname,clientname)==0)
		{
			q->next = p->next;
			//free(p);
			break;
		}
		q = p;
		p = p->next;
	}	
}		
