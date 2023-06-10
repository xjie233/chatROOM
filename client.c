#include<gtk/gtk.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<signal.h>
#include<pthread.h>
#include<semaphore.h>
#include<linux/types.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include"message.h"
#include"gettime.h"

sem_t bin_sem;
int client_sockfd;
const gchar *username;
GtkWidget *logionWindow;
GtkWidget *res_text_view;
GtkTextBuffer *view_buffer;

void accmsg(int *fd)                            //recive information from server
{	
	sem_wait(&bin_sem);
	char buf[50];
	char *text;					//storage information
	int clientfd = *fd,recvbytes;
	GtkTextBuffer *buffer;
	GtkTextIter start,end;
	text=(char *)malloc(50);
	while(1)
	{
		memset(buf,'\0',sizeof(buf));
		memset(text,'\0',sizeof(text));
		if((recvbytes=recv(clientfd,buf,sizeof(buf),0))<=0)
		{
			perror("recv");
			close(clientfd);		//close socked
			raise(SIGSTOP);
			exit(1);
		} 
		strcpy(text,buf);
		buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(res_text_view));
		gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer),&start,&end);
		gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer),&start,text,strlen(text));			
	}
}

void Clear_Local_message(gpointer *data)
{
	GtkTextIter start,end;
	GtkTextBuffer *send_buffer;
	send_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(data));
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(send_buffer),&start,&end);/*获得缓冲区开始和结束位置的Iter*/
	gtk_text_buffer_delete(GTK_TEXT_BUFFER(send_buffer),&start,&end);/*插入到缓冲区*/
}

void Put_Local_message(const gchar *text)
{
	GtkTextBuffer *rcv_buffer;
	GtkTextIter start,end;
	char *curtime;

	curtime = (char *)malloc(TIMELENGTH);   //give space
	rcv_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(res_text_view));
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(rcv_buffer),&start,&end);/*获得缓冲区开始和结束位置的Iter*/
	get_cur_time(curtime);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(rcv_buffer),&end,curtime,strlen(curtime));/*插入文本到缓冲区*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(rcv_buffer),&end," ",1);/*插入文本到缓冲区*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(rcv_buffer),&end,"我说:\n",8);/*插入文本到缓冲区*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(rcv_buffer),&end,text,strlen(text));/*插入文本到缓冲区*/
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(rcv_buffer),&end,"\n",1);/*插入文本到缓冲区*/
}

void Show_Err(char *err)
{
	GtkTextIter start,end;
	GtkTextBuffer *rcv_buffer;
	rcv_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(res_text_view));
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(rcv_buffer),&start,&end);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(rcv_buffer),&end,err,strlen(err));
}

void sendButton_event(GtkWidget *widget,gpointer *data)
{
	
	GtkTextBuffer *send_buffer;
	int sendbytes;
	GtkTextIter start,end;
	char *t;
	PMessage msg;
	t=(char *)malloc(TXTNUM);

	msg=(Message *)malloc(sizeof(Message));
	memset(msg,'\0',sizeof(Message));
	memset(t,'\0',sizeof(t));

	send_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(data));
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(send_buffer),&start,&end);
	const GtkTextIter s=start,e=end;
	t=gtk_text_buffer_get_text(GTK_TEXT_BUFFER(send_buffer),&s,&e,FALSE);
	
	if(strcmp(t,"")!=0)
	{
		strcpy(msg->username,username);
		strcpy(msg->type,"all");
		strcpy(msg->userpass,t);
		Clear_Local_message(data);
		if((sendbytes = send(client_sockfd,msg,sizeof(Message),0))==-1)
		{
			perror("send");
			exit(1);
		} 
		Put_Local_message(t);
	}
	else
	{
		Show_Err("消息不能为空....\n");
	}
}

/*关闭函数---------------------------------------------------------------------*/
void on_close(GtkButton *CloseButton,GtkWidget *window)
{
	gtk_main_quit();
}

void ChatLog()
{
	GtkWidget *window,*Send_scrolled_win;
	GtkWidget *vbox;
	GtkWidget *log_text_view;

	/*------------------------------绘制主窗口----------------------------*/
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);/*生成主窗口*/
	g_signal_connect(G_OBJECT(window),"delete_event",G_CALLBACK(gtk_main_quit),NULL);/*连接信号，关闭窗口*/
	gtk_window_set_title(GTK_WINDOW(window),"View Text");/*设置主窗口标题*/
	gtk_container_set_border_width(GTK_CONTAINER(window),10);/*设置主窗口边框*/
	gtk_widget_set_size_request(window,500,500);/*设置主窗口初始化大小*/
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);/*设置主窗口初始位置*/

	log_text_view = gtk_text_view_new();/*生成text view*/
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_text_view),GTK_WRAP_WORD);/*处理多行显示的模式*/
	gtk_text_view_set_justification(GTK_TEXT_VIEW(log_text_view),GTK_JUSTIFY_LEFT);/*控制文字显示方向的,对齐方式*/
	gtk_text_view_set_editable(GTK_TEXT_VIEW(log_text_view),FALSE);/*允许text view内容修改*/
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(log_text_view),TRUE);/*设置光标可见*/
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(log_text_view),5);/*设置上行距*/
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(log_text_view),5);/*设置下行距*/
	gtk_text_view_set_pixels_inside_wrap(GTK_TEXT_VIEW(log_text_view),5);/*设置词距*/
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(log_text_view),10);/*设置左边距*/
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(log_text_view),10);/*设置右边距*/
	view_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_text_view));

	vbox = gtk_vbox_new(FALSE,0);

	Send_scrolled_win = gtk_scrolled_window_new(NULL,NULL);/*生成滚动条的窗口*/
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Send_scrolled_win),log_text_view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Send_scrolled_win),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);/*滚动条属性*/
	gtk_container_add(GTK_CONTAINER(vbox),Send_scrolled_win);/*包装滚动条窗口到主窗口*/
	gtk_container_add(GTK_CONTAINER(window),vbox);/*将盒子封装到主窗口中去*/

	gtk_widget_show_all(window);/*显示所有东西*/
	gtk_main();/*主循环*/
}

void on_view(GtkButton *ViewButton,gpointer *data)
{
	PMessage msg;
	GtkTextIter start,end;
	int sendbytes;
	char chatlog[2048];
	strcpy(msg->username,username);
	strcpy(msg->userpass," ");
	strcpy(msg->type,"log");
	if((sendbytes = send(client_sockfd,msg,sizeof(Message),0))==-1)
	{
		perror("send");
		exit(1);
	} 
	if(recv(client_sockfd,chatlog,sizeof(chatlog),0)<=0)
	{
		perror("read:");
		close(client_sockfd);	//close client_socked
		raise(SIGSTOP);
		exit(1);
	}
	ChatLog();
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(view_buffer),&start,&end);
	gtk_text_buffer_insert(GTK_TEXT_BUFFER(view_buffer),&end,chatlog,strlen(chatlog));/*插入文本到缓冲区*/
}

void chatInterface()
{
	int res;
	res=sem_init(&bin_sem,0,0);
	GtkWidget *window/*定义主窗口*/,
          *Send_scrolled_win/*定义发送滚动窗口*/,
          *Rcv_scrolled_win/*定义接收滚动窗口*/;
	GtkWidget *vbox/*定义垂直盒子*/;
	GtkWidget *Button_Box/*定义按钮盒*/,
          *sendButton/*定义保存按钮*/,
          *CloseButton/*定义关闭按钮*/,
			*ViewButton;
	GtkWidget *hseparator/*定义水平分割线*/;
	GtkWidget *send_text_view;

	 /*------------------------------绘制主窗口----------------------------*/
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);/*生成主窗口*/
	g_signal_connect(G_OBJECT(window),"delete_event",G_CALLBACK(gtk_main_quit),NULL);/*连接信号，关闭窗口*/
	gtk_window_set_title(GTK_WINDOW(window),"Save Text");/*设置主窗口标题*/
	gtk_container_set_border_width(GTK_CONTAINER(window),10);/*设置主窗口边框*/
	gtk_widget_set_size_request(window,500,500);/*设置主窗口初始化大小*/
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);/*设置主窗口初始位置*/
        /*------------------------------设置Send_text view-------------------------*/
	send_text_view = gtk_text_view_new();/*生成text view*/
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(send_text_view),GTK_WRAP_WORD);/*处理多行显示的模式*/
	gtk_text_view_set_justification(GTK_TEXT_VIEW(send_text_view),GTK_JUSTIFY_LEFT);/*控制文字显示方向的,对齐方式*/
	gtk_text_view_set_editable(GTK_TEXT_VIEW(send_text_view),TRUE);/*允许text view内容修改*/
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(send_text_view),TRUE);/*设置光标可见*/
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(send_text_view),5);/*设置上行距*/
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(send_text_view),5);/*设置下行距*/
	gtk_text_view_set_pixels_inside_wrap(GTK_TEXT_VIEW(send_text_view),5);/*设置词距*/
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(send_text_view),10);/*设置左边距*/
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(send_text_view),10);/*设置右边距*/
	
        /*------------------------------设置Rcv_text view-------------------------*/
	res_text_view = gtk_text_view_new();/*生成text view*/
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(res_text_view),GTK_WRAP_WORD);/*处理多行显示的模式*/
	gtk_text_view_set_justification(GTK_TEXT_VIEW(res_text_view),GTK_JUSTIFY_LEFT);/*控制文字显示方向的,对齐方式*/
	gtk_text_view_set_editable(GTK_TEXT_VIEW(res_text_view),TRUE);/*允许text view内容修改*/
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(res_text_view),TRUE);/*设置光标可见*/
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(res_text_view),5);/*设置上行距*/
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(res_text_view),5);/*设置下行距*/
	gtk_text_view_set_pixels_inside_wrap(GTK_TEXT_VIEW(res_text_view),5);/*设置词距*/
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(res_text_view),10);/*设置左边距*/
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(res_text_view),10);/*设置右边距*/
	gtk_text_view_set_editable(GTK_TEXT_VIEW(res_text_view),FALSE);/*设置接收文字区不可被编辑*/
	 
    /*------------------------------设置发送窗口滚动条-------------------------------*/
	Send_scrolled_win = gtk_scrolled_window_new(NULL,NULL);/*生成滚动条的窗口*/
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Send_scrolled_win),send_text_view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Send_scrolled_win),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);/*滚动条属性*/
    /*------------------------------设置接收窗口滚动条-------------------------------*/
	Rcv_scrolled_win = gtk_scrolled_window_new(NULL,NULL);/*生成滚动条的窗口*/
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Rcv_scrolled_win),res_text_view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Rcv_scrolled_win),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);/*滚动条属性*/
    /*------------------------------设置垂直盒子------------------------------*/
	vbox = gtk_vbox_new(FALSE,0);/*生成一个垂直排布的盒子*/
    /*------------------------------设置发送按钮------------------------------*/
	sendButton = gtk_button_new_with_label("发送");/*生成发送按钮*/
	g_signal_connect(G_OBJECT(sendButton),"clicked",G_CALLBACK(sendButton_event),(gpointer)send_text_view);/*给按钮加上回调函数*/
    /*------------------------------设置关闭按钮------------------------------*/
	CloseButton = gtk_button_new_with_label("关闭");/*生成关闭按钮*/
	g_signal_connect(G_OBJECT(CloseButton),"clicked",G_CALLBACK(on_close),(gpointer)window);

	ViewButton=gtk_button_new_with_label("聊天记录");
	g_signal_connect(G_OBJECT(ViewButton),"clicked",G_CALLBACK(on_view),(gpointer)window);
    /*------------------------------设置按钮盒子------------------------------*/     
	Button_Box = gtk_hbutton_box_new();/*生成按钮盒*/
	gtk_box_set_spacing(GTK_BOX(Button_Box),1);/*按钮之间的间隔*/
	gtk_button_box_set_layout(GTK_BUTTON_BOX(Button_Box),GTK_BUTTONBOX_END);/*按钮盒内部布局，风格是尾对齐*/
	gtk_container_set_border_width(GTK_CONTAINER(Button_Box),5);/*边框宽*/
    /*------------------------------设置分割线--------------------------------*/
	hseparator = gtk_hseparator_new();
    /*------------------------------设置分割面板------------------------------*/
	gtk_container_add(GTK_CONTAINER(vbox),Rcv_scrolled_win);/*包装滚动条窗口到主窗口*/
	gtk_container_add(GTK_CONTAINER(vbox),hseparator);/*加入一条分割线*/
	gtk_container_add(GTK_CONTAINER(vbox),Send_scrolled_win);/*包装滚动条窗口到主窗口*/    
	gtk_container_add(GTK_CONTAINER(vbox),Button_Box);/*把按钮盒包装到vbox中*/
	gtk_box_pack_start(GTK_BOX(Button_Box),CloseButton,TRUE,TRUE,5);/*把关闭按钮包装到按钮盒里面去*/
	gtk_box_pack_start(GTK_BOX(Button_Box),sendButton,TRUE,TRUE,5);/*把发送按钮包装到按钮盒里面去*/
	gtk_box_pack_start(GTK_BOX(Button_Box),ViewButton,TRUE,TRUE,5);/*把发送按钮包装到按钮盒里面去*/
	gtk_container_add(GTK_CONTAINER(window),vbox);/*将盒子封装到主窗口中去*/    
    /*------------------------------显示所有东西------------------------------*/
	gtk_widget_show_all(window);/*显示所有东西*/
	sem_post(&bin_sem);
	gtk_main();/*主循环*/
}

int bindPort(unsigned short int port)
{
	struct sockaddr_in clientaddr;		//定义地址结构
	int clientfd,sendbytes;				//客户端文件描述符
  	struct hostent *host;				//存储主机信息的数据结构			
	//host = gethostbyname(hostname);

        if((clientfd=socket(AF_INET,SOCK_STREAM,0))==-1)    //AF_INET------unix网络套接字    SOCK_STREAM----可靠面向连接的双向字节流
	{
		perror("socket:");
		exit(1);
	}
	//客户端填充服务器资料
	clientaddr.sin_family = AF_INET;
        clientaddr.sin_port  = htons(port);
	clientaddr.sin_addr.s_addr   = inet_addr("127.0.0.1");
	bzero(&(clientaddr.sin_zero),0);
	if(connect(clientfd,(struct sockaddr *)&clientaddr,sizeof(struct sockaddr))==-1)
	{
		perror("connect");
		exit(1);
	}
	return clientfd;
}

void closeApp(GtkWidget *window,gpointer data)
{
	gtk_main_quit();	
}

void btnCancle_clicked(GtkWidget *button,gpointer data)
{
	gtk_main_quit();
}

void button_clicked(GtkWidget *button,GtkWidget *gtkwidget[])
{
	int sendbytes;				//client send 
	int count=0;			//logion n numbers
	const gchar *text1;
       pthread_t  sendpid,recvpid;
	PMessage msg;
	char *buf,*keyword,*filename,*distname,tmparr[3][TXTNUM]={"all","\0","\0"};
	int warntype=-1;
	buf= (char *)malloc(TXTNUM);						//Here must apply space for buf
       keyword=(char *)malloc(TXTNUM);				                //Here must apply space for keyword
	memset(buf,0,TXTNUM);
	msg=(Message *)malloc(sizeof(Message));
	memset(msg,'\0',sizeof(Message));
	client_sockfd=bindPort((unsigned short int)MYPORT);
	GtkWidget *dialog=gtk_message_dialog_new((GtkWindow *)gtkwidget[0],GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO, " 用户名不存在或密码错误");
	strcpy(msg->type,"login");
	username=gtk_entry_get_text(GTK_ENTRY((GtkWidget *)gtkwidget[1]));
       text1=gtk_entry_get_text(GTK_ENTRY((GtkWidget *)gtkwidget[2]));
	strcpy(msg->username,username);
	strcpy(msg->userpass,text1);
	if((sendbytes = send(client_sockfd,msg,sizeof(Message),0))==-1)
	{
		perror("send");
		exit(1);
	} 
	if(recv(client_sockfd,&warntype,sizeof(int),0)<=0)
	{
		perror("read:");
		close(client_sockfd);	//close client_socked
		raise(SIGSTOP);
		exit(1);
	}
	if(warntype==LOGSUCCESS)
	{
		gtk_widget_hide(logionWindow);
		pthread_create(&recvpid,NULL,(void *)accmsg,(void *)&client_sockfd); 
		chatInterface();
	}
	else
	{		  
		gtk_dialog_run(GTK_DIALOG(dialog));      	
	}
}

void main(int argc,char *argv[])
{
	GtkWidget *window;
	GtkWidget *username_label,*password_label;
	GtkWidget *username_entry,*password_entry;
	GtkWidget *ok_button,*Register;
	GtkWidget *cancle;
	GtkWidget *hbox1,*hbox2,*hbox3,*hbox4;
	GtkWidget *vbox;
	GtkWidget *gtkwidget[3];
	gtk_init(&argc,&argv);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window),"CHAT");
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window),200,300);
	g_signal_connect(GTK_OBJECT(window),"destroy",GTK_SIGNAL_FUNC(closeApp),NULL);
	username_label=gtk_label_new("username");
	password_label=gtk_label_new("password");
	username_entry=gtk_entry_new();
	password_entry=gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password_entry),FALSE);
	gtk_entry_set_invisible_char(GTK_ENTRY(password_entry),(gchar)'*');
	ok_button=gtk_button_new_with_label("登录");
	cancle=gtk_button_new_with_label("取消");
	Register=gtk_button_new_with_label("注册");
	gtkwidget[0]=window;
	logionWindow=window;
	gtkwidget[1]=username_entry;
	gtkwidget[2]=password_entry;
	g_signal_connect(GTK_OBJECT(ok_button),"clicked",GTK_SIGNAL_FUNC(button_clicked),gtkwidget);
	g_signal_connect(GTK_OBJECT(cancle),"clicked",GTK_SIGNAL_FUNC( btnCancle_clicked),gtkwidget);
	//g_signal_connect(GTK_OBJECT(ok_button),"clicked",GTK_SIGNAL_FUNC(resButton_clicked),gtkwidget);
	hbox1=gtk_hbox_new(TRUE,5);
	hbox2=gtk_hbox_new(TRUE,5);
	hbox3=gtk_hbox_new(TRUE,5);
	hbox4=gtk_hbox_new(TRUE,5);
	vbox=gtk_vbox_new(FALSE,15);
	gtk_box_pack_start(GTK_BOX(hbox1),username_label,TRUE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox1),username_entry,TRUE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox2),password_label,TRUE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox2),password_entry,TRUE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox3),Register,TRUE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox3),ok_button,TRUE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox3),cancle,TRUE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox1,FALSE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox2,FALSE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox3,FALSE,FALSE,5);
	gtk_container_add(GTK_CONTAINER(window),vbox);
	gtk_widget_show_all(window);
	gtk_main();
	
}
