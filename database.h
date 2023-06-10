#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"getlocalip.h"
#include"mysql.h"
#include"message.h"

#define TRUE 1
#define FALSE 0

int res;

MYSQL_RES *res_ptr;
MYSQL_ROW sqlrow;
MYSQL my_connection;

int connectdatabase()  //0 is right,1 is rong
{
	mysql_init(&my_connection);
	if(mysql_real_connect(&my_connection,"localhost","cwt","037","CHAt",0,NULL,0))
	{
		return 0;
	}
	else
	{
		fprintf(stderr,"Connection failed\n");
		if(mysql_errno(&my_connection))
		{
			fprintf(stderr,"Connection error %d: %s\n",mysql_errno(&my_connection),mysql_error(&my_connection));
		}
		return 1;
	}
}

int UpdateIp(char *id)   //update ip use userid,
{
	char ip[20];
	char qs[1024];
	getlocalip(ip);
	sprintf(qs,"update Login_table set userip='%s' where userid='%s'",ip,id);
	if(connectdatabase()==1)
	{
		printf("connect database error...\n");
		exit(0);
	}
	res=mysql_query(&my_connection,qs);
	if(res)
	{
		fprintf(stderr,"update error %d: %s",mysql_errno(&my_connection),mysql_error(&my_connection));
		return 1;
	}
	return 0;
}

int InserteUser(PMessage msg)
{
	char qs[1024];
	sprintf(qs,"insert into Login_table(userid,userpass) values('%s','%s')",msg->username,msg->userpass);
	if(connectdatabase()==1)
	{
		printf("connect database error...\n");
		exit(0);
	}
	res=mysql_query(&my_connection,qs);
	if(res)
	{
		fprintf(stderr,"update error %d: %s",mysql_errno(&my_connection),mysql_error(&my_connection));
		return 1;
	}
	return 0;	
}

//reurn 1 is exist
int IsExits(PMessage sourcemsg)
{
	char qs[1024];
	sprintf(qs,"select * from Login_table where userid='%s'",sourcemsg->username);
	if(connectdatabase()==1)
	{
		printf("connect database error...\n");
		exit(0);
	}
	res=mysql_query(&my_connection,qs);
	if(res)
	{
		fprintf(stderr,"update error %d: %s",mysql_errno(&my_connection),mysql_error(&my_connection));
		exit(0);
	}
	res_ptr=mysql_store_result(&my_connection);
	if(res_ptr)
	{
		sqlrow=mysql_fetch_row(res_ptr);
		if(sqlrow==0)                            //not exist
			return 0;
		else
			return 1;
	}
	exit(0);
}

int LoginCheck(PMessage sourcemsg)
{
	char qs[1024];
	sprintf(qs,"select * from Login_table where userid='%s' and userpass='%s'",sourcemsg->username,sourcemsg->userpass);
	if(connectdatabase()==1)
	{
		printf("connect database error...\n");
		exit(0);
	}
	res=mysql_query(&my_connection,qs);
	if(res)
	{
		fprintf(stderr,"update error %d: %s",mysql_errno(&my_connection),mysql_error(&my_connection));
		exit(0);
	}
	res_ptr=mysql_store_result(&my_connection);
	if(res_ptr)
	{
		sqlrow=mysql_fetch_row(res_ptr);
		if(sqlrow==0)
			return FALSE;
		return TRUE;
	}
	exit(0);
}
