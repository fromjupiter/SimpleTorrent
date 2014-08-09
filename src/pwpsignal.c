#include "pwpsignal.h"
#include <stdbool.h>
#include <errno.h>

int num_peer=0;//最大链接的peer数目,最大为4
// 如果成功接收报文, 返回1, 否则返回-1.
 int* piece_num;
int read_n( int fd, char *bp, size_t len)
{
    int cnt;
    int rc;

    cnt = len;
    while ( cnt > 0 )
    {
        rc = recv( fd, bp, cnt, 0 );
        if ( rc < 0 )				/* read error? */
        {
            if ( errno == EINTR )	/* interrupted? */
                continue;			/* restart the read */
            return -1;				/* return error */
        }
        if ( rc == 0 )				/* EOF? */
            return len - cnt;		/* return short count */
        bp += rc;
        cnt -= rc;
    }
    return len;
}

void add_peer(peer_t* peer)
{
    if(peers_head==NULL)
    {
        peers_head=peer;
        return;
    }
    peer->next=peers_head->next;
    peers_head->next=peer;
    return;
}

void del_peer(int sockfd)
{
    if(peers_head!=NULL)
    {
        if(peers_head->next==NULL)
        {
            if(peers_head->sockfd==sockfd)
            {
                free(peers_head);
                peers_head=NULL;
                return;
            }
        }
        else
        {
            peer_t* t=peers_head;
            if(t->next->next!=NULL)
            {
                if(t->next->sockfd==sockfd)
                {
                    peer_t* temp=t->next;
                    t->next=temp->next;
                    free(temp);
                    return;
                }
            }
        }
    }
}

peer_t* get_peer(int sockfd)
{
    peer_t* t=peers_head;
    while(t!=NULL)
    {
        if(t->sockfd==sockfd)
            return t;
        t=t->next;
    }
    return NULL;
}
int peer_in(char* peer_id)
{
    int num=g_tracker_response->numpeers;
    int i=0;
    for(; i<num; i++)
    {
        if(memcmp(peer_id,g_tracker_response->peers[i].id,21)==0)
            return 1;
    }
    return -1;

}
/*用来实现最小分片*/
//根据bitfiled更新分片数目
void count_piecenum(char* bitfield){
    int i=0;
    for(;i<g_num_pieces;i++){
      if(bitfield[i]==1){
        piece_num[i]++;
      }
    }
    return;
}
//选择最小分片
int get_leastindex(char* bitfield){
  int i=0;
  int j=0;
    int index=0;
    for(;j<g_num_pieces;j++)
    {
        if(bitfield[j]==1){
            index=j;
            break;
        }
    }
    if(j==g_num_pieces)
    return -1;
 // int index=0;
  int flag=0;  //标记有没有找到
  for(;i<g_num_pieces;i++)
        if(piece_num[index]>=piece_num[i]&&bitfield[i]==1&&bitfield_local[i]==0&&
          piece_length_num[i]==0&&piece_index_current[i]==0)
        {
          index=i;
          flag=1;
        }
  if(flag==1)
    return index;
  return -1;
}
int handshake(int sockfd)
{
    handshake_msg* hs_msg=(handshake_msg*)malloc(sizeof(handshake_msg));
    hs_msg->pstrlen=PSTRLEN;
    strcpy(hs_msg->pstr,"BitTorrent protocol");
    memcpy(&(hs_msg->info_hash),&g_infohash,20);
	  hs_msg->info_hash[0]=htonl(hs_msg->info_hash[0]);
	  hs_msg->info_hash[1]=htonl(hs_msg->info_hash[1]);
	  hs_msg->info_hash[2]=htonl(hs_msg->info_hash[2]);
	  hs_msg->info_hash[3]=htonl(hs_msg->info_hash[3]);
	  hs_msg->info_hash[4]=htonl(hs_msg->info_hash[4]);
    memcpy(&(hs_msg->peer_id),&g_my_id,20);
    memset(&(hs_msg->reserved),0,8);
    //memset(hs_msg->peer_id,g_my_id,20);
    //int sockfd=connect_to_host(ip,port);
    printf("start send!! sockfd是多少 %d\n",sockfd);
    if((send(sockfd,hs_msg,sizeof(handshake_msg),0))<=0)
    {
        printf("handshake error\n");
        free(hs_msg);
        hs_msg=NULL;
        return -1;
    }
    printf("发送消息成功\n");
    //free(hs_msg);
    //hs_msg=NULL;
    //接受握手消息
    //没有用到老师的接收函数
    char recvline[MAXLINE];
	sleep(2);
    if(read_n(sockfd,recvline,sizeof(handshake_msg))<=0)
    {
        printf("handshake error \n");
        free(hs_msg);
        hs_msg=NULL;
        printf("--------握手失败---------\n");
        return -1;
    }
    printf("----------------收到握手消息--------------------\n");
	//printf("%d  \n",((handshake_msg*)recvline)->pstrlen);
	//printf("%d \n",ntohl(((handshake_msg*)recvline)->pstrlen));
	((handshake_msg*)recvline)->info_hash[0]=ntohl(((handshake_msg*)recvline)->info_hash[0]);
	((handshake_msg*)recvline)->info_hash[1]=ntohl(((handshake_msg*)recvline)->info_hash[1]);
	((handshake_msg*)recvline)->info_hash[2]=ntohl(((handshake_msg*)recvline)->info_hash[2]);
	((handshake_msg*)recvline)->info_hash[3]=ntohl(((handshake_msg*)recvline)->info_hash[3]);
	((handshake_msg*)recvline)->info_hash[4]=ntohl(((handshake_msg*)recvline)->info_hash[4]);
    if((((handshake_msg*)recvline)->pstrlen)==PSTRLEN&&memcmp(((handshake_msg*)recvline)->info_hash,g_infohash,20)==0&&memcmp(g_my_id,((handshake_msg*)recvline)->peer_id,20)!=0) //是握手消息的回应
    {
         printf("-----------------握手消息正确-------------------\n");
        //收到对方peer对握手消息的相应，开始进行进一步的通信
        free(hs_msg);
        return 1;
    }
    return -1;
}
//该线程用来在握手成功之后,进行进一步的交互，该子线程包括主动的握手过程
//其中connect_to_host函数已经先于之前被调用，所以传递进来的参数中包括sockfd
void*  handshake_pwp(void* arg)
{
    printf("start handshake_pwp!!!\n");
    printf("++++++++++++++++++++++  %d\n",num_peer);
     thread_arg* arg_t=(thread_arg*)arg;
     if(num_peer>6){
     //   close(arg_t->sockfd);
        pthread_exit(NULL);
    }
    printf("-------------- %s\n",arg_t->peer_ip);
     arg_t->sockfd=connect_to_host(arg_t->peer_ip,arg_t->peer_port);
    //进行握手过程
    if(arg_t->sockfd<0){
      printf("connect error\n");
      pthread_exit(NULL);
    }
    if(num_peer>6){
        close(arg_t->sockfd);
        pthread_exit(NULL);
    }
    if(handshake(arg_t->sockfd)<0)
    {
        close(arg_t->sockfd);
        pthread_exit(NULL);
    }
    printf("++++handshake success!+++\n");
    //在全局的peer池中进行登记
    //初始态
    //name 怎么初始化？
    peer_t* peer_tcb=(peer_t*)malloc(sizeof(peer_t));
    peer_tcb->choking=0;
    peer_tcb->choked=0;
    peer_tcb->interested=0;
    peer_tcb->have_interest=0;
    peer_tcb->sockfd=arg_t->sockfd;
    peer_tcb->next=NULL;
    add_peer(peer_tcb);
    num_peer++;
    //查找是否有bitfield要发
    //数据部分
    //以下未写，由刘铖同学完成
    //
    printf("handshake_pwp sockfd : %d\n",arg_t->sockfd);
int i;
    bool can_send_bitfield=false;
    for(i=0; i<g_num_pieces; i++)
    {
        if(bitfield_local[i]==1)
        {
            can_send_bitfield=true;
        }
    }
    if(can_send_bitfield==true)
    {
        int bit_num=g_num_pieces/8;
        if(g_num_pieces%8!=0)
            bit_num++;
        peer_payload *bitfield_send=(peer_payload*)malloc(5+bit_num);
        bitfield_send->length=htonl((1+bit_num));
//		bitfield_send->payload=(char*)malloc(bit_num*sizeof(char));
    char *buff=(char*)malloc(bit_num*sizeof(char));
    bitfield_send->payload=buff;
        bitfield_send->message=5;
		printf("send_payload!!\n");
        int i=0;
        char bit_8=0x80;
        for(;i<g_num_pieces;i++){
            if(bitfield_local[i]==1){
                *(buff)=*(buff)|bit_8;
            }
            bit_8=bit_8>>1;
            if((i+1)%8==0){
                (buff)++;
                bit_8=0x80;
            }
                
        }
        //send_payload(arg_t->sockfd,(char*)bitfield_send);
		free(bitfield_send->payload);
		free(bitfield_send);
    }
    payloadhandler(arg_t->sockfd);//
	free(peer_tcb);
    free(arg_t);
    pthread_exit(NULL);
}


//等待后来者接入。传递的参数只是监听套接字
void* wait_pwp(void* arg)
{
    printf("wait_pwp!!!\n");
    int listen_sockfd=*((int*)arg);
	printf("listen_sockfd %d\n",listen_sockfd);
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    int connfd;
    for(;;)
    {
		clilen=sizeof(cliaddr);
		printf("!!!!!!!!!!waitadasdas\n");
        connfd=accept(listen_sockfd,(struct sockaddr*)&cliaddr,&clilen);
		printf("!!!!!!!!!!wait----------\n");
		//printf("connfd----  %d\n",connfd); 
        if(connfd>=0){
			printf("connfd 是 %d\n",connfd);
			/*peer_t* peer_tcb=(peer_t*)malloc(sizeof(peer_t));
			peer_tcb->choking=0;
			peer_tcb->choked=0;
			peer_tcb->interested=0;
			peer_tcb->have_interest=0;
			peer_tcb->sockfd=connfd;
			peer_tcb->next=NULL;
			add_peer(peer_tcb);*/
			//name 怎么初始化？
			//接收握手消息
			pthread_t tid;
            printf("jinlaile+++++++++++++++++++++++\n");
			if(pthread_create(&tid,NULL,peer_enter,(void*)&connfd)!=0){ 
				printf("Error in creating wait_pwp thread\n");
				close(listen_sockfd);
				pthread_exit(NULL);
			}
        }
    }
	close(listen_sockfd);
    pthread_exit(NULL);
}
//本地peer作为监听者，如果连接成功对每一个外部连接都新建此类线程，
//参数为套接字描述符
void* peer_enter(void* arg)
{
    printf("peer_enter!!\n");
    //int index=*((int*)arg);
    char recvline[MAXLINE];
    int connfd=*(int*)arg;
	printf("connfd   %d------------\n",connfd);
    peer_t* peer_tcb=(peer_t*)malloc(sizeof(peer_t));
    peer_tcb->choking=0;
    peer_tcb->choked=0;
    peer_tcb->interested=0;
    peer_tcb->have_interest=0;
    peer_tcb->sockfd=connfd;
    peer_tcb->next=NULL;
    //name 怎么初始化？
    //接受握手消息
	printf("peer_enter sockfd %d \n",connfd);
    if(read_n(connfd,recvline,sizeof(handshake_msg))>0)
    {
    
       printf("-------------收到握手消息-------------------\n");
	   printf("%d  \n",((handshake_msg*)recvline)->pstrlen);
	((handshake_msg*)recvline)->info_hash[0]=ntohl(((handshake_msg*)recvline)->info_hash[0]);
	((handshake_msg*)recvline)->info_hash[1]=ntohl(((handshake_msg*)recvline)->info_hash[1]);
	((handshake_msg*)recvline)->info_hash[2]=ntohl(((handshake_msg*)recvline)->info_hash[2]);
	((handshake_msg*)recvline)->info_hash[3]=ntohl(((handshake_msg*)recvline)->info_hash[3]);
	((handshake_msg*)recvline)->info_hash[4]=ntohl(((handshake_msg*)recvline)->info_hash[4]);
        if((((handshake_msg*)recvline)->pstrlen)==PSTRLEN&&memcmp(((handshake_msg*)recvline)->info_hash,g_infohash,20)==0&&memcmp(g_my_id,((handshake_msg*)recvline)->peer_id,20)!=0)  //   收到握手消息
        {
            printf("-----------发出握手回应消息----------\n");
            handshake_msg* hs_msg=(handshake_msg*)malloc(sizeof(handshake_msg));
	((handshake_msg*)recvline)->info_hash[0]=htonl(((handshake_msg*)recvline)->info_hash[0]);
	((handshake_msg*)recvline)->info_hash[1]=htonl(((handshake_msg*)recvline)->info_hash[1]);
	((handshake_msg*)recvline)->info_hash[2]=htonl(((handshake_msg*)recvline)->info_hash[2]);
	((handshake_msg*)recvline)->info_hash[3]=htonl(((handshake_msg*)recvline)->info_hash[3]);
	((handshake_msg*)recvline)->info_hash[4]=htonl(((handshake_msg*)recvline)->info_hash[4]);
            memcpy(hs_msg,recvline,sizeof(handshake_msg));
            memcpy(hs_msg->peer_id,g_my_id,20); //给对方回复握手消息
            if((send(connfd,hs_msg,sizeof(handshake_msg),0))<=0)
            {
                printf("handshake error");
                free(hs_msg);
                hs_msg=NULL;
                pthread_exit(NULL);
            }
        }
    }
    //之后是消息
    //bitfield之类的
    //由刘铖同学完成
    //
    //
    //
    //
    //
    //check_file(g_torrentmeta->name);
    add_peer(peer_tcb);
    int i;
    bool can_send_bitfield=false;
    for(i=0; i<g_num_pieces; i++)
    {
        if(bitfield_local[i]==1)
        {
            can_send_bitfield=true;
        }
    }
    if(can_send_bitfield==true)
    {
        int bit_num=g_num_pieces/8;
        if(g_num_pieces%8!=0)
            bit_num++;
        peer_payload *bitfield_send=(peer_payload*)malloc(5+bit_num);
        bitfield_send->length=htonl((1+bit_num));
//		bitfield_send->payload=(char*)malloc(bit_num*sizeof(char));
    char *buff=(char*)malloc(bit_num*sizeof(char));
    bitfield_send->payload=buff;
        bitfield_send->message=5;
		printf("send_payload!!\n");
        int i=0;
        char bit_8=0x80;
        for(;i<g_num_pieces;i++){
            if(bitfield_local[i]==1){
                *(buff)=*(buff)|bit_8;
            }
            bit_8=bit_8>>1;
            if((i+1)%8==0){
                (buff)++;
                bit_8=0x80;
            }
                
        }
        send_payload(connfd,(char*)bitfield_send);
		free(bitfield_send->payload);
		free(bitfield_send);
    }
    printf("asasasa\n");
    payloadhandler(connfd);//这个参数应该是peer_t
    pthread_exit(NULL);
}




