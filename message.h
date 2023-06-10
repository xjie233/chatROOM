#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define TIMELENGTH 20         //时间字符串所用长度
#define MENUM 3    	         //菜单的种类个数
#define UNUM  20              //用户名的最大位数
#define TXTNUM 50              //密码的最大长度
#define LOGIN 1               //登录
#define REGISTE 2            //注册
#define QUIT  3              //退出
#define MAXLINE 1024 
#define MYPORT  3492         //端口号
#define FILENAMELENGTH 20    //文件名的长度
#define TYPELENGTH 10	 //类型的长度

#define PERM S_IRUSR|S_IWUSR
#define BACKLOG 10 		//宏定义，定义服务程序可以连接的最大客户数量

typedef enum WARNMSG
{
		REGSUCCESS,		//注册成功
		USEREXIST,		//注册时用户名存在
		RSGKEYWORD,		//注册时是系统关键字
		LOGSUCCESS,		//登陆成功
		ALREADYLOG,		//用户已经登录
		USERORPWDERR,	//用户名不存在或密码错误
		OTHER,			//其他原因导致登录或注册失败
		NOTONLINE	 	//此用户不在线
}Warntype;	

typedef struct message{                     //消息
	char username[UNUM];		//用户名
	char userpass[TXTNUM];		//密码
	char type[TYPELENGTH];		//类型：登录、注册、退出。。。
}Message,* PMessage;

char *warnmsg[]={"注册成功","注册时用户名存在","注册时是系统关键字","登陆成功","用户已经登录","用户名不存在或密码错误","其他原因导致登录或注册失败","此用户不在线"};
char *sysword[]={"all","regist","trans","login","view","personal","quit","end",};

int isSysword(char *word)       //判断是否为系统关键字
{
	int i=0;
	while(strcmp(sysword[i],"end")!=0)
	{
		if(strcmp(sysword[i],word)==0)
		{
			return 1;
		}
		i++;
	}
	return 0;
}

//分界函数，功能是将用户输入的命令分开
int explode(char arr[][TXTNUM],char *buf,char ch)
{
	int count=0,i,j,k=0;
	char *tmp=NULL,*buf_c=NULL;
	tmp = (char *)malloc(100);
	for(buf_c=buf;(*buf_c)!='\0';buf_c++)
	{
		if(*buf_c==ch) 
			count++;
		if(count==2) 
			break;
	}
	if(count==1)
	{
		for(i=0,buf_c = buf;*buf_c!='\0';buf_c++,i++)
		{
			*(tmp+i) = *buf_c;
			if(*buf_c==ch) 
			{
				buf_c++;
				break;
			}
		}
		*(tmp+i)='\0';
		strcpy(arr[0],tmp);
		for(i=0;*buf_c!='\0';buf_c++,i++)
		{
			*(tmp+i) = *buf_c;
		}
		*(tmp+i)='\0';
		strcpy(arr[1],tmp);
	} 
	else if(count==2)
	{
		for(i=0,j=0,buf_c = buf;*buf_c!='\0';buf_c++,i++)
		{
	
			if(*buf_c==ch) 
			{
				*(tmp+i)='\0';
				strcpy(arr[j],tmp);
				j++,i=0,buf_c++;;
				if(j==count) 
				{break;}
			}
			*(tmp+i) = *buf_c;
		}
		for(i=0;*buf_c!='\0';buf_c++,i++)
		{
			*(tmp+i) = *buf_c;
		}
		*(tmp+i)='\0';
		strcpy(arr[j],tmp);		
	} 
	else if(count==0)
	{
		strcpy(arr[1],buf);
		return 1;
	}
	return count;
}
