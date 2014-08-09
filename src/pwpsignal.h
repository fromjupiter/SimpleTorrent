
#ifndef __PWPSIGNAL_H__
#define __PWPSIGNAL_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "btdata.h"
#include "bencode.h"
#include "util.h"

#define PSTRLEN 19
#define RESERVEDLEN 8
//#define MAXLINE 1024
#define MAXPEERS 100  //最大连接100个PEER

//握手消息的结构体
typedef struct  handshake_msg{
   unsigned char  pstrlen;
   //char* pstr;
   char pstr[PSTRLEN];
   unsigned char reserved[RESERVEDLEN];
   int info_hash[5];
   char peer_id[20];
}handshake_msg;

peer_t* peers_head;  //用来连接所有与本地peer链接的其他peer
peer_t* get_peer(int sockfd); //通过套接字来得到peer_t
void del_peer(int sockfd); //通过套接字来删除peer_t  
void add_peer(peer_t* peer);//添加到全局的链表中

//int peer_num;//用来统计现在有多少个peer和本地有连接,在主函数前边被初始化
//peer_t  peer_pool[MAXPEERS]; //用来管理所有的peer之间的连接状态，最大数目有限定

//整个握手过程是一个阻塞过程，包括发送和接受
//extern int handshake(char* ip,int port);
extern int* piece_num;  //用来计数每个片远端peer的数目 
extern int handshake(int sockfd);
extern int peer_in(char*);  //判断某个peer是否在trakce返回的所有peer中，在,返回1，否则-1
void* peer_enter(void* arg);
void* wait_pwp(void* arg);
void*  handshake_pwp(void* arg);
extern void count_piecenum(char*);  //计数拥有各分片的peer数目
extern int get_leastindex(char*); //返回最小分片的编号

#endif 
