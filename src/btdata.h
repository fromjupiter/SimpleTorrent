
#ifndef __BTDATA_H__
#define __BTDATA_H__
#include <pthread.h>
#include "bencode.h"
#include<stdbool.h>

/**************************************
 * 一些常量定义
**************************************/
/*
#define HANDSHAKE_LEN 68  // peer握手消息的长度, 以字节为单位
#define BT_PROTOCOL "BitTorrent protocol"
#define INFOHASH_LEN 20
#define PEER_ID_LEN 20
#define MAXPEERS 100
#define KEEP_ALIVE_INTERVAL 3
*/

#define BT_STARTED 0
#define BT_STOPPED 1
#define BT_COMPLETED 2
#define PIECE_MAX_LEN 16384
/**************************************
 * 数据结构
**************************************/
// Tracker HTTP响应的数据部分
typedef struct _tracker_response {
  int size;       // B编码字符串的字节数
  char* data;     // B编码的字符串
} tracker_response;

// 元信息文件中包含的数据
typedef struct _torrentmetadata {
  int info_hash[5]; // torrent的info_hash值(info键对应值的SHA1哈希值)
  char* announce; // tracker的URL
  int length;     // 文件长度, 以字节为单位
  char* name;     // 文件名
  int piece_len;  // 每一个分片的字节数
  int num_pieces; // 分片数量(为方便起见)
  char* pieces;   // 针对所有分片的20字节长的SHA1哈希值连接而成的字符串
} torrentmetadata_t;

// 包含在announce url中的数据(例如, 主机名和端口号)
typedef struct _announce_url_t {
  char* hostname;
  int port;
} announce_url_t;

// 由tracker返回的响应中peers键的内容
typedef struct _peerdata {
  char id[21]; // 20用于null终止符
  int port;
  char* ip; // Null终止
} peerdata;

// 包含在tracker响应中的数据
typedef struct _tracker_data {
  int interval;
  int numpeers;
  peerdata* peers;
} tracker_data;

typedef struct _tracker_request {
  int info_hash[5];
  char peer_id[20];
  int port;
  int uploaded;
  int downloaded;
  int left;
  char ip[16]; // 自己的IP地址, 格式为XXX.XXX.XXX.XXX, 最后以'\0'结尾
} tracker_request;

// 针对到一个peer的已建立连接, 维护相关数据
typedef struct _peer_t {
  int sockfd;
  int choking;        // 作为上传者, 阻塞远端peer
  int interested;     // 远端peer对我们的分片有兴趣
  int choked;         // 作为下载者, 我们被远端peer阻塞
  int have_interest;  // 作为下载者, 对远端peer的分片有兴趣
  char name[20]; 
  struct _peer_t* next;
} peer_t;
typedef struct thread_arg{
    char peer_id[20];
    int peer_port;
    char peer_ip[16];
    int sockfd;

}thread_arg;
typedef struct _peer_t_payload{
	int length;
	char message;
	char *payload;
} peer_payload;
typedef struct _peer_t_payload_alive{
	int length;
}peer_payload_alive;
typedef struct _peer_t_payload_normal{
	int length;
	char message;
} peer_payload_normal;
typedef struct _peer_t_payload_have{
	int length;
	char message;
	int piece_index;
} peer_payload_have;
typedef struct _peer_t_payload_request{
	int length;
	char message;
	int index;
	int begin;
	int piece_length;
} peer_payload_request;
typedef struct _peer_t_payload_piece{
	int length;
	char message;
	int index;
	int begin;
	char *data;
}peer_payload_piece;
typedef struct _peer_t_payload_cancel{
	int length;
	char message;
	int index;
	int begin;
	int piece_length;
}peer_payload_cancel;
/**************************************
 * 全局变量 
**************************************/
char g_my_ip[128]; // 格式为XXX.XXX.XXX.XXX, null终止
int g_peerport; // peer监听的端口号
int g_infohash[5]; // 要共享或要下载的文件的SHA1哈希值, 每个客户端同时只能处理一个文件
char g_my_id[20];

int g_done; // 表明程序是否应该终止

torrentmetadata_t* g_torrentmeta;
char* g_filedata;      // 文件的实际数据
int g_filelen;
int g_num_pieces;
char g_tracker_ip[16]; // tracker的IP地址, 格式为XXX.XXX.XXX.XXX(null终止)
int g_tracker_port;
tracker_data *g_tracker_response;

// 这些变量用在函数make_tracker_request中, 它们需要在客户端执行过程中不断更新.
int g_uploaded;
int g_downloaded;
int g_left;
int sockfd_traker;

char *bitfield_local;//<<<<<<<<<<本地的文件标志
//初始化文件，如果文件为空则产生临时文件，否则判定文件sha值
int file_init(char *name);
//将文件分片读取，判断sha值，初始化bitfield_local
void check_file(char *name);
int send_payload(int sockfd,char *payload);
char *SHA1_piece(int index);
void payloadhandler(int sockfd) ;  //这个参数应该是peer_t
bool SHA_COMP(unsigned *sha_value,char* p);
int piece_length_num[PIECE_MAX_LEN];//<<<<<<<<<<<<<片的个数数组，每一位对应其以及发送的长度
extern int piece_num_current;
extern pthread_mutex_t count_mutex;
extern char piece_index_current[PIECE_MAX_LEN];
char last_piece[409600];
#endif
