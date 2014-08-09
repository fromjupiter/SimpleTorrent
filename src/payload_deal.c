/*************************************************************************
	> File Name: payload_deal.c
	> Author: liucheng
	> Mail: 1035377294@qq.com
	> Created Time: Wed 28 May 2014 05:26:06 PM CST
 ************************************************************************/

#include<stdio.h>
#include "util.h"
#include "sha1.h"
#include "pwpsignal.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <errno.h>
//payload发送函数，同时输出发送信息
#define ALIVETIME 120000000 //2分钟
#define MAX_PIECE_LEN 409600


//返回SHA1字符指针(没有用堆区，返回了一个地址，注意比较时只比较20个字节且不要做出对元数据的修改，index为分片下标)
char *SHA1_piece(int index)
{
    char *p = g_torrentmeta->pieces;
    int i = 0;
    for(; i < index; i++)
    {
        p += 20;
    }
    return p;
}


int send_payload(int sockfd,char *payload)
{
    char buf[MAX_PIECE_LEN];
    memcpy(buf,&(((peer_payload*)payload)->length),sizeof(int));
    if(ntohl(((peer_payload* )payload)->length)>0)
        memcpy(buf+4,&(((peer_payload*)payload)->message),sizeof(char));
    if(ntohl(((peer_payload*)payload)->length)>1){
		if(((peer_payload*)payload)->message==4){
			memcpy(buf+5,&(((peer_payload_have*)payload)->piece_index),sizeof(int));
		}
        else if(((peer_payload*)payload)->message==5){
            memcpy(buf+5,((peer_payload*)payload)->payload,ntohl(((peer_payload*)payload)->length)-1);
        }
        else if(((peer_payload*)payload)->message==6){
            memcpy(buf+5,&(((peer_payload_request*)payload)->index),sizeof(int));
            memcpy(buf+9,&(((peer_payload_request*)payload)->begin),sizeof(int));
            memcpy(buf+13,&(((peer_payload_request*)payload)->piece_length),sizeof(int));
        }
        else if(((peer_payload*)payload)->message==7){
            memcpy(buf+5,&(((peer_payload_piece*)payload)->index),sizeof(int));
            memcpy(buf+9,&(((peer_payload_piece*)payload)->begin),sizeof(int));
            memcpy(buf+13,((peer_payload_piece*)payload)->data,ntohl(((peer_payload*)payload)->length)-9);
        }
        else if(((peer_payload*)payload)->message==8){
            memcpy(buf+5,&(((peer_payload_request*)payload)->index),sizeof(int));
            memcpy(buf+9,&(((peer_payload_request*)payload)->begin),sizeof(int));
            memcpy(buf+13,&(((peer_payload_request*)payload)->piece_length),sizeof(int));
        }
    }
	printf("have!!!!   %d   - --  - -\n",sockfd);
	
    if(send(sockfd,buf,sizeof(int)+ntohl(((peer_payload*)payload)->length)*sizeof(char),0)<0)
    {
        printf("======send_payload error=====\n");
        return -1;
    }
    printf("======send_payload success======\n");  //alive消息没有message部分
    printf("          length : %d \n",ntohl(((peer_payload*)payload)->length));
    if(ntohl(((peer_payload*)payload)->length)>0)
    {
        printf("            id   : %d \n",(int)((peer_payload*)payload)->message);
        if(((peer_payload*)payload)->message==4)
            printf("      have piece : %d\n",ntohl(((peer_payload_have*)payload)->piece_index));
        else if(((peer_payload*)payload)->message==5){
            printf("      bitfield   : %s",((peer_payload*)payload)->payload);
			int k;
			for(k=0;k<ntohl(((peer_payload*)payload)->length)-1;k++)
				printf("%d ",((peer_payload*)payload)->payload[k]);
			printf("\n");
		}
        else if(((peer_payload*)payload)->message==6)
        {
            printf("  request index  : %d\n",ntohl(((peer_payload_request*)payload)->index));
            printf("  request begin  : %d\n",ntohl(((peer_payload_request*)payload)->begin));
            printf(" request length  : %d\n",ntohl(((peer_payload_request*)payload)->piece_length));
        }
        else if(((peer_payload*)payload)->message==7)
        {
            printf("    piece index  : %d\n",ntohl(((peer_payload_piece*)payload)->index));
            printf("    piece begin  : %d\n",ntohl(((peer_payload_piece*)payload)->begin));
        }
        else if(((peer_payload*)payload)->message==8)
        {
            printf("   cancel index  : %d\n",ntohl(((peer_payload_cancel*)payload)->index));
            printf("   cancel begin  : %d\n",ntohl(((peer_payload_cancel*)payload)->begin));
            printf("  cancel length  : %d\n",ntohl(((peer_payload_cancel*)payload)->piece_length));
        }
    }
    return 1;
}
// 如果成功接收报文, 返回1, 否则返回-1.
int readn( int fd, char *bp, size_t len)
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
//payload就收函数，同时输出发送信息
//要根据收到的消息来判断
int recv_payload(int sockfd,peer_payload *payload)
{
    int n;
      //  printf("======start recv_payload =====\n");
    if((n=readn(sockfd,(char*)payload,sizeof(int)))>0)  //先接收长度
    {
        printf("======recv_payload success=====\n");
        payload->length=ntohl(payload->length);
        printf("       length : %d\n",payload->length);
        if(payload->length>0)
        {
            char *buf=(char*)malloc(sizeof(char));
            //memset(buf,0,1);
            if((n=readn(sockfd,buf,sizeof(char)))>0)
            {
                if(payload->length==1){
                    payload->message=buf[0];
                    printf("            id   : %d \n",(int)payload->message);
                }
                else
                {
                    char *recv_buf=(char *)malloc(payload->length-1);
					payload->payload=(char*)malloc((payload->length-1)*sizeof(char));
					if((n=readn(sockfd,recv_buf,(payload->length-1)*sizeof(char)))>0){
						payload->message=buf[0];
						//payload->payload=buf+1;
						printf("            id   : %d \n",(int)payload->message);
						if(payload->message==4)
						{
                            int a=*(int*)&recv_buf[0];
                            a=ntohl(a);
							((peer_payload_have*)payload)->piece_index=a;
							printf("      have piece : %d\n",(int)((peer_payload_have*)payload)->piece_index);
						}
						else if(payload->message==5){
                            memcpy(payload->payload,recv_buf,payload->length-1);
							printf("      bitfield   : %s\n",payload->payload);           //要强制类型转换的把？
						}
						else if(payload->message==6)
						{
                            int index=*(int*)&recv_buf[0];
                            index=ntohl(index);
							((peer_payload_request*)payload)->index=index;//ntohl(((peer_payload_request*)payload)->index);
                            int begin=*(int*)&recv_buf[4];
                            begin=ntohl(begin);
							((peer_payload_request*)payload)->begin=begin;//ntohl(((peer_payload_request*)payload)->begin);
                            int piece_length=*(int*)&recv_buf[8];
                            piece_length=ntohl(piece_length);
							((peer_payload_request*)payload)->piece_length=piece_length;//ntohl(((peer_payload_request*)payload)->piece_length);
							printf("  request index  : %d\n",(int)((peer_payload_request*)payload)->index);
							printf("  request begin  : %d\n",(int)((peer_payload_request*)payload)->begin);
							printf(" request length  : %d\n",(int)((peer_payload_request*)payload)->piece_length);
						}
						else if(payload->message==7)
						{
                            int index=*(int*)&recv_buf[0];
                            index=ntohl(index);
							((peer_payload_piece*)payload)->index=index;//ntohl(((peer_payload_piece*)payload)->index);
                            int begin=*(int*)&recv_buf[4];
                            begin=ntohl(begin);
							((peer_payload_piece*)payload)->begin=begin;//ntohl(((peer_payload_piece*)payload)->begin);
                            ((peer_payload_piece*)payload)->data=recv_buf+8;
                            printf("    piece index  : %d\n",(int)((peer_payload_piece*)payload)->index);
							printf("    piece begin  : %d\n",(int)((peer_payload_piece*)payload)->begin);
						}
						else if(payload->message==8)
						{
                            int index=*(int*)&recv_buf[0];
                            index=ntohl(index);
							((peer_payload_cancel*)payload)->index=index;//ntohl(((peer_payload_cancel*)payload)->index);
                            int begin=*(int*)&recv_buf[4];
                            begin=ntohl(begin);
							((peer_payload_cancel*)payload)->begin=begin;//=ntohl(((peer_payload_cancel*)payload)->begin);
                            int piece_length=*(int*)&recv_buf[8];
                            piece_length=ntohl(piece_length);
							((peer_payload_cancel*)payload)->piece_length=piece_length;//h=ntohl(((peer_payload_cancel*)payload)->piece_length);
							printf("   cancel index  : %d\n",(int)((peer_payload_cancel*)payload)->index);
							printf("   cancel begin  : %d\n",(int)((peer_payload_cancel*)payload)->begin);
							printf("  cancel length  : %d\n",(int)((peer_payload_cancel*)payload)->piece_length);
						}
					}
                    //要加interest和choke 的
                    //******
                    //*******
                    //*****
                    //******
                }
        printf("======end recv_payload =====\n");
                return 1;
            }
			free(buf);
        }
        else{
        printf("======end recv_payload =====\n");
            return 1;
    }}
  //  printf("======recv_payload error=====\n");
    return -1;
}

//int file_length = g_filelen;//<<<<<<<<<<<<<<<<<<<<<<<写文件长度，另外写
//char *filename = g_torrentmeta->name;//<<<<<<<<<<<<<<<<<<<文件名字，通过torrent获取
//char *bitfield_local;//<<<<<<<<<<本地的文件标志
//int piece_num = g_num_pieces;//<<<<<<<<<<<,片的个数
//int piece_length_num[MAX_PIECE_NUM];//<<<<<<<<<<<<<片的个数数组，每一位对应其以及发送的长度
//int piece_length = g_torrentmeta->piece_len;//<<<<<<<<<<<<<片的大小
//线程，处理payload的接受函数
int piece_num_current=0;
//bool is_cancle=false;
pthread_mutex_t count_mutex;
char piece_index_current[PIECE_MAX_LEN];
int last_piece_length=0;
//初始化piece_length_num数组
void init_piece_length(int piece_length_num[], int num)
{
    int i = 0;
    for(; i < num; i++)
        piece_length_num[i] = i * (g_torrentmeta->piece_len);
}

void payloadhandler(int sockfd)   //这个参数应该是peer_t
{
    //int piece_length_num[g_num_pieces];//<<<<<<<<<<<<<片的个数数组，每一位对应其以及发送的长度
	char *bitfield=(char*)malloc(g_num_pieces);
    peer_payload *payload=(peer_payload *)malloc(sizeof(peer_payload));
int cancel_index[PIECE_MAX_LEN];
int cancel_num=0;

    //char *payload;
    memset(piece_length_num,0,g_num_pieces);
    memset(payload,0,sizeof(payload));
    char piece[g_torrentmeta->piece_len];
    int piece_index;
    int send_index_length;
    //int sockfd=(int)arg;
	int m=0;
	for(;m<g_num_pieces;m++){
		if(bitfield_local[m]==1){
		        bitfield[m]=1;
			piece_length_num[m]=g_torrentmeta->piece_len;
		}
		else
		        bitfield[m]=0;
		if(m==g_num_pieces-1&&bitfield_local[m]==1)
			piece_length_num[m]=g_filelen-(g_num_pieces-1)*g_torrentmeta->piece_len;
	}
    clock_t start=clock();
     clock_t end=clock();
    while(1)
    {
        //
        //在这里加时钟，时钟可以定义为全局的,如果大于2分钟，就杀死线程
        //收到alive信息，则时钟置为此刻时间
        //时钟可以是局部的，在该函数内部
        //
		//printf("sockfd : %d \n",sockfd);
        if(clock()-start>=ALIVETIME){
            printf("recv time out error!!!\n");
            close(sockfd);
            return;
        }
        if(clock()-end>=ALIVETIME){
           //发送leep——alive
           end=clock();
           peer_payload_alive* keep_alive=(peer_payload_alive*)malloc(sizeof(peer_payload_alive));
           send_payload(sockfd,(char*)keep_alive);
           free(keep_alive);
        }
        if(recv_payload(sockfd,payload)>0)
        {
			printf("!!!!!!!!!!!!qweqweqwewq!\n");
            if(((peer_payload*)payload)->length==0) //keep-alive
            {
                //收到alive函数
                // 则会判断你
                printf("live !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                start=clock();

            }
            else if(((peer_payload*)payload)->length>0)
            {
                switch(((peer_payload*)payload)->message)   //choke等消息要修改全局的状态peer_t
                {
                case 0: //choke
                {
                    //修改状态
                    peer_t* peer=get_peer(sockfd);
                    peer->choked=1;

                    break;
                }
                case 1: //unchoke
                {
                    //修改状态
                    printf("!!!!!!!!!!!!11111\n");
                    peer_t* peer=get_peer(sockfd);
                    peer->choked=0;
                    //发送request
                    send_index_length=0;
					//int n=0;
					pthread_mutex_lock(&count_mutex);
                    int piece_len_to_send;
                    for(piece_index=0;piece_index<g_num_pieces;piece_index++)
                    {
                        if(piece_num_current==g_num_pieces-1&&bitfield[piece_index]==1
                           &&bitfield_local[piece_index]==0){
                            piece_index_current[piece_index]=1;
                            int j=0;
                            send_index_length=0;
                            if(piece_index==g_num_pieces-1)
                                piece_len_to_send=g_filelen-(g_num_pieces-1)*(g_torrentmeta->piece_len) ;
                            else piece_len_to_send=g_torrentmeta->piece_len;
                            printf("!!!!!!!!!!!!2222\n");
                            for(; ;)
                            {
						        usleep(1);
                                if(piece_len_to_send-send_index_length>PIECE_MAX_LEN)
                                {
                                    peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                                    request->length=htonl(13);
                                    request->message=6;
                                    request->index=htonl(piece_index);
                                    request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                                    request->piece_length=htonl(PIECE_MAX_LEN);  //<<<<<<<<<<<<<<<<<<<<
                                    send_payload(sockfd,(char*)request);
                                    free(request);
                                    send_index_length+=PIECE_MAX_LEN;
                                }
                                else
                                {
                                    peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                                    request->length=htonl(13);
                                    request->message=6;
                                    request->index=htonl(piece_index);
                                    request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                                    request->piece_length=htonl(piece_len_to_send-send_index_length);  //<<<<<<<<<<<<<<<<<<<<
                                    send_payload(sockfd,(char*)request);
                                    free(request);
                                    send_index_length=piece_len_to_send;
                                    break;
                                }
                            }
					        pthread_mutex_unlock(&count_mutex);
                            continue;
                        }
                        //piece_index=rand()%g_num_pieces;
                        if(piece_index==get_leastindex(bitfield)&&get_leastindex(bitfield)!=-1){
								//&&bitfield[piece_index]==1&&bitfield_local[piece_index]==0
							    //&&piece_length_num[piece_index]==0
								//&&piece_index_current[piece_index]==0)
          printf("-------------最少分片！分片号%d----------------\n",piece_index);
							break;
					}}
					//piece_index=get_leastindex(bitfield);
					//printf("!!!!!!!!!!piece_index: %d\n",piece_index);
					//if(piece_index==-1)break;
					if(piece_index==g_num_pieces){
					
					    pthread_mutex_unlock(&count_mutex);
                        break;
                    }
					piece_index_current[piece_index]=1;
					pthread_mutex_unlock(&count_mutex);
                    int j=0;
                    send_index_length=0;
                    if(piece_index==g_num_pieces-1)
                       piece_len_to_send=g_filelen-(g_num_pieces-1)*(g_torrentmeta->piece_len) ;
                    else piece_len_to_send=g_torrentmeta->piece_len;
                    printf("!!!!!!!!!!!!2222\n");
                    for(; j<5; j++)
                    {
						usleep(1);
                        if(piece_len_to_send-send_index_length>PIECE_MAX_LEN)
                        {
                            peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                            request->length=htonl(13);
                            request->message=6;
                            request->index=htonl(piece_index);
                            request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                            request->piece_length=htonl(PIECE_MAX_LEN);  //<<<<<<<<<<<<<<<<<<<<
                            send_payload(sockfd,(char*)request);
                            free(request);
                            send_index_length+=PIECE_MAX_LEN;
                        }
                        else
                        {
                            peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                            request->length=htonl(13);
                            request->message=6;
                            request->index=htonl(piece_index);
                            request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                            request->piece_length=htonl(piece_len_to_send-send_index_length);  //<<<<<<<<<<<<<<<<<<<<
                            send_payload(sockfd,(char*)request);
                            free(request);
                            
                            
                            
                            send_index_length=piece_len_to_send;
                            break;
                        }
                    }
                    break;
                }
                case 2: //interested
                {
                    //   发送是否阻塞
                    //以及修改状态
                    peer_t* peer=get_peer(sockfd);
                    peer->interested=1;
                    if(peer->choking==1)  //阻塞                //??????????????????????????/  1shishenme
                    {
                        peer_payload_normal *chock_piece =(peer_payload_normal *)malloc(sizeof(peer_payload_normal));
                        chock_piece->length=htonl(1);
                        chock_piece->message=0;
                        send_payload(sockfd,(char*)chock_piece);

                    }
                    else
                    {
                        peer_payload_normal *unchock_piece =(peer_payload_normal *)malloc(sizeof(peer_payload_normal));
                        unchock_piece->length=htonl(1);
                        unchock_piece->message=1;
                        send_payload(sockfd,(char*)unchock_piece);

                    }
                    break;
                }
                case 3: //uninterested
                {
                    // 修改状态
                    //
                    peer_t* peer=get_peer(sockfd);
                    peer->interested=0;
                    break;
                }
                case 4: //have ，判断该分片是否是自己想要的，如果是发送interested
                {
                    peer_payload_have *payload_have=(peer_payload_have*)payload;
					bitfield[payload_have->piece_index]=1;
					piece_num[payload_have->piece_index]++;
                    if(bitfield_local[payload_have->piece_index]==0&&send_index_length==0)
                    {
                        //send interest
                        peer_payload_normal *interest =(peer_payload_normal *)malloc(sizeof(peer_payload_normal));
                        interest->length=htonl(1);
                        interest->message=2;
                        send_payload(sockfd,(char*)interest);
                    }
                    break;
                }
                case 5: //bitfield，如果想要发送 interested
                {
                    /*if((payload->length-1)!= g_num_pieces) //<<<<<<<<<<<<<<
                    {
                        close(sockfd);
                        return ;
                    }*/
                       //收到bitfield则判断是否感兴趣，感兴趣则发interested
                    {
                        char *bit_8=((peer_payload*)payload)->payload;//循环发request

						int i;
						for(i=0;i<(((peer_payload*)payload)->length-1)*8;i++){
							if((*bit_8 & 0x80)!=0){
								bitfield[i]=1;
							}
							else bitfield[i]=0;
							*bit_8=*bit_8<<1;
							if((i+1)%8==0)
								bit_8++;
						}
                        for(i=0;i<g_num_pieces;i++)
                            printf("%d ",bitfield[i]);
                        ////!!!!!!!!!send interest
                        /*peer_payload_normal *interest =(peer_payload_normal *)malloc(sizeof(peer_payload_normal));
                        interest->length=htonl(1);
                        interest->message=2;
                        send_payload(sockfd,(peer_payload*)interest);
						free(interest);*/
						count_piecenum(bitfield);
					}
					
                    int k=0;
                    bool is_interest=false;
                    for(k=0; k<g_num_pieces; k++){
						printf("\n  %d    %d\n",bitfield[k],bitfield_local[k]);
                        if(bitfield[k]==1&&bitfield_local[k]==0)
                        {
                            is_interest=true;
                            break;
                        }
                    }
                    if(is_interest==true)
                    {
                        peer_payload_normal *interest =(peer_payload_normal *)malloc(sizeof(peer_payload_normal));
                        interest->length=htonl(1);
                        interest->message=2;
                        send_payload(sockfd,(char*)interest);
						free(interest);
                    }
                    else
                    {
                        peer_payload_normal *uninterest =(peer_payload_normal *)malloc(sizeof(peer_payload_normal));
                        uninterest->length=htonl(1);
                        uninterest->message=3;
                        send_payload(sockfd,(char*)uninterest);
						free(uninterest);
                    }
                    break;
                }
                case 6: //request
                {
                    peer_payload_request *payload_request=(peer_payload_request*)payload;
                    if(payload_request->piece_length>131072)
                    {
                        close(sockfd);
                        return ;
                    }
                    else //收到request发送分片
                    {
						pthread_mutex_lock(&count_mutex);
                        printf("request!!!!!!!!!!\n");
                        peer_payload_piece *payload_piece=(peer_payload_piece*)malloc(sizeof(peer_payload_piece));
                        payload_piece->data=(char*)malloc(payload_request->piece_length);
                        payload_piece->length=htonl(9+payload_request->piece_length);
                        payload_piece->message=7;
                        payload_piece->index=htonl(payload_request->index);
                        payload_piece->begin=htonl(payload_request->begin);
                        FILE *f=fopen(g_torrentmeta->name,"r");
						printf("before seek!!!\n");
                        fseek(f,(payload_request->index)*g_torrentmeta->piece_len+payload_request->begin,SEEK_SET);
						printf("seek!!!!\n");
                        printf("read %d  !!!!!!!!!!\n",payload_request->piece_length);
                        fread(payload_piece->data,sizeof(char),payload_request->piece_length,f);
                        printf("end read!!!!!!!!!!\n");
                        send_payload(sockfd,(char*)payload_piece);
                        fclose(f);
                        free(payload_piece->data);
                        free(payload_piece);
                    pthread_mutex_unlock(&count_mutex);
                    }
                    break;
                }
                case 7: //piece
                {
                    pthread_mutex_lock(&count_mutex);
                    peer_payload_piece *payload_piece=(peer_payload_piece*)payload;
                    if(piece_num_current==g_num_pieces)
                        break;
                    if(piece_num_current==g_num_pieces-1){
                        peer_payload_cancel *payload_cancel=(peer_payload_cancel*)malloc(sizeof(peer_payload_cancel));
                        payload_cancel->length=htonl(13);
                        payload_cancel->message=8;
                        payload_cancel->index=htonl(payload_piece->index);
                        payload_cancel->begin=htonl(payload_piece->begin);
                        payload_cancel->piece_length=htonl(payload_piece->length-9);
                        peer_t *temp_peer=peers_head;
					    while(temp_peer!=NULL){
                            if(temp_peer->sockfd!=sockfd)
					            send_payload(temp_peer->sockfd,(char*)payload_cancel);
						    temp_peer=temp_peer->next;
						}
                        free(payload_cancel);
                        if(memcmp(last_piece+payload_piece->begin,payload_piece->data,payload_piece->length-9)!=0){
						    memcpy(last_piece+payload_piece->begin,payload_piece->data,payload_piece->length-9);
                            last_piece_length+=(payload_piece->length-9);
                        }
                    }
                   else{ 
                        memcpy(piece+payload_piece->begin,payload_piece->data,payload_piece->length-9);
                        piece_length_num[payload_piece->index]+=(payload_piece->length-9);
				   }
                    printf("%s\n",payload_piece->data);
                    int piece_len_to_send;
                    if(payload_piece->index==g_num_pieces-1)
                        piece_len_to_send=g_filelen-(g_num_pieces-1)*(g_torrentmeta->piece_len) ;
                    else piece_len_to_send=g_torrentmeta->piece_len;
                    printf("last_piece_length !!! %d\n",last_piece_length);
                    if(piece_length_num[payload_piece->index]==piece_len_to_send||
                      last_piece_length==piece_len_to_send)
                    {
                        printf("piece max!!!\n");
                        //片接受完整，判断sha值
                        unsigned sha_value[5];
                        SHA1Context sha;
                        SHA1Reset(&sha) ;
						if(piece_num_current==g_num_pieces-1)
							SHA1Input(&sha,(const unsigned char *)last_piece,piece_len_to_send);
						else
							SHA1Input(&sha,(const unsigned char *)piece,piece_len_to_send);
                        if(!SHA1Result(&sha))
                        {
                            fprintf(stderr,"ERROR-- could not compute meassage digest\n");
                        }
                        else
                        {
                            printf("\t");
                            int i;
                            /*<<<<<<< HEAD
                                                        for(i=0; i<5; i++)
                                                        {
                                                            sprintf(temp,"%X ",sha.Message_Digest[i]);
                            =======*/
                            for(i=0; i<5; i++)
                            {
                                sha_value[i]=htonl(sha.Message_Digest[i]);
								printf("%X ",sha.Message_Digest[i]);
                            }
                        }
						char *p=SHA1_piece(payload_piece->index);
                            if(SHA_COMP(sha_value,p)) //<<<<<<<<<<<<<<<<<<<<<<s
                            {
                                printf("write file !!!!!\n");
                                //这里“”写torrent读取的sha值，表明sha值正确，写入文件
                                FILE *f=fopen(g_torrentmeta->name,"r+");
                                fseek(f,(payload_piece->index)*g_torrentmeta->piece_len,SEEK_SET);
								if(piece_num_current==g_num_pieces-1)
									fwrite(last_piece,1,piece_len_to_send,f);
								else 
									fwrite(piece,1,piece_len_to_send,f);
                                fclose(f);
                                printf("close file  !!!!!!!!!\n");
                                bitfield_local[payload_piece->index]=1;
                                peer_payload_have *payload_have=(peer_payload_have*)malloc(sizeof(peer_payload_have));
                                payload_have->length=htonl(5);
                                payload_have->message=4;
                                payload_have->piece_index=htonl(payload_piece->index);
                                int i=0;
                                printf("send have!!!!!!!!\n");
                                peer_t *temp_peer=peers_head;
								while(temp_peer!=NULL){
                                    //printf("send_payload(temp_peer->sockfd,(char*)payload_have)\n");
					                //printf("sockfd  %d\n",temp_peer->sockfd);
									if(temp_peer->sockfd!=sockfd)
                                        send_payload(temp_peer->sockfd,(char*)payload_have);
									temp_peer=temp_peer->next;
								}
                                piece_num_current++;
                                printf("free have!!!!!!!!\n");
                                free(payload_have);
                            }
							else{
								piece_length_num[payload_piece->index]=0;
                                piece_index_current[payload_piece->index]=0;
							}
                        
                    }
                    pthread_mutex_unlock(&count_mutex);
                    //表明分片接受完毕，重新请求分片
                    if(piece_num_current==g_num_pieces)
                        break;
                    if(send_index_length==piece_len_to_send)
                    {
                        printf("reget pieces!!!\n");
                        int n;
                        bool is_full=true;
                        for(n=0;n<g_num_pieces;n++){
                            if(bitfield_local[n]==1)
                                continue;
                            else {
                                is_full=false;
                                break;
                            }
                            if(n==g_num_pieces-1)
                                is_full=false;
                        }
                        if(is_full==true){
                            printf("recv file is full!!!!\n");
							getchar();
                            break;
                        }
						pthread_mutex_lock(&count_mutex);
                        //send request for other piece
						for(piece_index=0;piece_index<g_num_pieces;piece_index++)
                        //while(1)
                        {
                            if(piece_num_current==g_num_pieces-1&&bitfield[piece_index]==1
                           &&bitfield_local[piece_index]==0){
                                piece_index_current[piece_index]=1;
                                int j=0;
                                send_index_length=0;
                                if(piece_index==g_num_pieces-1)
                                    piece_len_to_send=g_filelen-(g_num_pieces-1)*(g_torrentmeta->piece_len) ;
                                else piece_len_to_send=g_torrentmeta->piece_len;
                                printf("!!!!!!!!!!!!2222\n");
                                for(; ;)
                                {
						            usleep(1);
                                    if(piece_len_to_send-send_index_length>PIECE_MAX_LEN)
                                    {
                                        peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                                        request->length=htonl(13);
                                        request->message=6;
                                        request->index=htonl(piece_index);
                                        request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                                        request->piece_length=htonl(PIECE_MAX_LEN);  //<<<<<<<<<<<<<<<<<<<<
                                        send_payload(sockfd,(char*)request);
                                        free(request);
                                        send_index_length+=PIECE_MAX_LEN;
                                    }
                                    else
                                    {
                                        peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                                        request->length=htonl(13);
                                        request->message=6;
                                        request->index=htonl(piece_index);
                                        request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                                        request->piece_length=htonl(piece_len_to_send-send_index_length);  //<<<<<<<<<<<<<<<<<<<<
                                        send_payload(sockfd,(char*)request);
                                        free(request);
                                        send_index_length=piece_len_to_send;
                                        break;
                                    }
                                }
					            pthread_mutex_unlock(&count_mutex);

                                continue;
                            }
                            //piece_index=(rand()%g_num_pieces);
                            printf("piece_index %d  piece_length_num  %d   piece_index_current %d\n",piece_index,piece_length_num[piece_index],piece_index_current[piece_index]);
                            if(piece_index==get_leastindex(bitfield)&&get_leastindex(bitfield)!=-1){
									//&&bitfield[piece_index]==1&&bitfield_local[piece_index]==0
									//&&piece_length_num[piece_index]==0&&
									//piece_index_current[piece_index]==0)
          printf("-------------最少分片！分片号%d----------------\n",piece_index);
                                break;
						}}
						if(piece_index==g_num_pieces){
						
					        pthread_mutex_unlock(&count_mutex);
                            break;
                        }
						piece_index_current[piece_index]=1;
						pthread_mutex_unlock(&count_mutex);
                        send_index_length=0;
                    }
                    //五个请求接受完毕，重新发送请求
                       // int piece_len_to_send;
                        if(piece_index==g_num_pieces-1)
                            piece_len_to_send=g_filelen-(g_num_pieces-1)*(g_torrentmeta->piece_len) ;
                        else piece_len_to_send=g_torrentmeta->piece_len;
                    if(send_index_length==payload_piece->begin+payload_piece->length-9||send_index_length==0)
                    {
                        int j=0;
                        for(; j<5; j++)
                        {
							usleep(1);
                            if(piece_len_to_send-send_index_length>PIECE_MAX_LEN)
                            {
                                peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                                request->length=htonl(13);
                                request->message=6;
                                request->index=htonl(piece_index);
                                request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                                request->piece_length=htonl(PIECE_MAX_LEN);  //<<<<<<<<<<<<<<<<<<<<
                                send_payload(sockfd,(char*)request);
                                free(request);
                                send_index_length+=PIECE_MAX_LEN;
                            }
                            else
                            {
                                peer_payload_request *request=(peer_payload_request*)malloc(sizeof (peer_payload_request));
                                request->length=htonl(13);
                                request->message=6;
                                request->index=htonl(piece_index);
                                request->begin=htonl(send_index_length);//<<<<<<<<<<<<<
                                request->piece_length=htonl(piece_len_to_send-send_index_length);  //<<<<<<<<<<<<<<<<<<<<
                                send_payload(sockfd,(char*)request);
                                free(request);
                                send_index_length=piece_len_to_send;
                                break;
                            }
                        }
                    }
                    break;
                }
                case 8: //cancel
                {
                    peer_payload_cancel *payload_cancel=(peer_payload_cancel*)payload;
                    cancel_index[cancel_num]=payload_cancel->begin;
                    cancel_num++;
                    //is_cancel=true;
                    //change the state to choke
                    break;
                }
                default:
                {
                    printf("length message  error!!!!!!!!\n");
                    break;
                }
                }
            }
        }
    }
    free(payload);
	pthread_exit(NULL);
    return;
}
