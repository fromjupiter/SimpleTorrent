#include "btdata.h"
#include "util.h"
#include "stdbool.h"
// 读取并处理来自Tracker的HTTP响应, 确认它格式正确, 然后从中提取数据. 
// 一个Tracker的HTTP响应格式如下所示:
// (I've annotated it)
// HTTP/1.0 200 OK       (17个字节,包括最后的\r\n)
// Content-Length: X     (到第一个空格为16个字节) 注意: X是一个数字
// Content-Type: text/plain (26个字节)
// Pragma: no-cache (18个字节)
// \r\n  (空行, 表示数据的开始)
// data                  注意: 从这开始是数据, 但并没有一个data标签
tracker_response* preprocess_tracker_response(int sockfd)
{ 
   char recvline[MAXLINE];
   char tmp[MAXLINE];
   char* data;
   int len;
   int offset = 0;
   int datasize = -1;
   printf("Reading tracker response...\n");
   // HTTP LINE
   len = recv(sockfd,recvline,17,0);
   if(len < 0)
   {
     perror("Error, cannot read socket from tracker");
     exit(-6);
   }
   //printf("recvline: %s\n", recvline);

   strncpy(tmp, recvline, 17);
   if(strncmp(tmp, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK")) == 0) {
	   
	   memset(recvline,0xFF,MAXLINE);//
	   memset(tmp,0x0,MAXLINE);
   
	   //Content-type
	   len = recv(sockfd,recvline,26,0);
	   if(len <= 0){
		   perror("Error, cannot read socket from tracker");
		   exit(-6);
	   }
   
	   // Content-Length
	   len = recv(sockfd,recvline,16,0);
	   if(len <= 0){
		   perror("Error, cannot read socket from tracker");
		   exit(-6);
	   }
	   strncpy(tmp,recvline,16);
	   printf("tmp: %s\n", tmp);
	   if(strncmp(tmp,"Content-Length: ", strlen("Content-Length: ")) != 0)
	   {
		   perror("Error, didn't match Content-Length line");
		   exit(-6);
	   }
	   // printf("recvline: %s\n", recvline);
	   memset(recvline,0xFF,MAXLINE);
	   memset(tmp,0x0,MAXLINE);
	  
	   // 读取Content-Length的数据部分
	   char c[2];
	   char num[MAXLINE];
	   int count = 0;
	   c[0] = 0; c[1] = 0;
	   while(c[0] != '\r' && c[1] != '\n') {
		   len = recv(sockfd,recvline,1,0);
		   if(len <= 0) {
			   perror("Error, cannot read socket from tracker");
			   exit(-6);
		   }
		   num[count] = recvline[0];
		   c[0] = c[1];
		   c[1] = num[count];
		   count++;
	   }
	   printf("num: %s", num);
	   datasize = atoi(num);
	   printf("NUMBER RECEIVED: %d\n",datasize);
	   memset(recvline,0xFF,MAXLINE);
	   memset(num,0x0,MAXLINE);
  
	   // 读取Content-type和Pragma行
	   /*len = recv(sockfd,recvline,26,0);
		 if(len <= 0) {
		 perror("Error, cannot read socket from tracker");
		 exit(-6);
		 }
	    len = recv(sockfd,recvline,18,0);
		if(len <= 0) {
		perror("Error, cannot read socket from tracker");
		exit(-6);
		}*/
   // 去除响应中额外的\r\n空行
   
	   len = recv(sockfd,recvline,2,0);
	   if(len <= 0) {
		   perror("Error, cannot read socket from tracker");
		   exit(-6);
	   }
   // 分配空间并读取数据, 为结尾的\0预留空间
   
	   int i, j = 0, m = 0, count_peers = 0;//记录tracker返回的peers数量 
	   char buf[datasize];
	   data = (char*)malloc(MAXLINE);
	   bool peer = false, peers_ip = false;
	   char num_peers[10], *ip_port;
	   
	   len = recv(sockfd, buf, datasize, 0);
	   if(len < 0) {
		   perror("Error, cannot read socket from tracker\n");
		   exit(-6);
	   }
	
	   for(i = 0; i < datasize; i++) {
	  
		   if(peer == true) {
			   if(buf[i] != ':') { 
				   num_peers[m] = buf[i];
				   m++;
			   }
			   else {
				   count_peers = atoi(num_peers)/6;
				   printf("count_peers: %d\n", count_peers);
				   peers_ip = true;
				   data[j] = 'l';
				   j++;
				   continue;
			   }
		   }
		   else {
			   data[j] = buf[i];
			   j++;
		   }
		   if(peers_ip == true) {
			   printf("count_peers: %d\n", count_peers);
			   if(count_peers > 0) {
				   ip_port = &data[j];
				   char ip[16];
				   int ip_len, port;
				   unsigned int *p = (unsigned int *)&buf[i+4];
				   port = ntohs(*p);//通过指针计算端口号

				   struct in_addr ip_s;//提取ip地址
				   ip_s.s_addr = (*(int *)(&buf[i]));
				   printf("s_addr: %s\n", inet_ntoa(ip_s));
				   ip_len = strlen(inet_ntoa(ip_s));
				   sprintf(ip_port, "d7:peer id1:a2:ip%d:%s4:porti%dee", ip_len, inet_ntoa(ip_s), port);//id写成了单个字母a
				   i += 5;//切记不是加6
				   j += strlen(ip_port);
				   count_peers--;//列表中的项数
			   }
		   }
		   else if(i > 4) {
			   if(buf[i] == 's' && buf[i-1] == 'r' && buf[i-2] == 'e' && buf[i-3] == 'e' && buf[i-4] == 'p') {
				   peer = true;
			   }
		   }
	   }
	   data[j] = 'e'; data[j + 1] = 'e';
	   datasize = j + 2;
	   printf("datasize: %d\n", datasize);
	   data[datasize] = '\0';
	   for(i=0; i < datasize; i++)
		   printf("%c",data[i]);
	   printf("\n");

	   // 分配, 填充并返回tracker_response结构
	   tracker_response* ret;
	   ret = (tracker_response*)malloc(sizeof(tracker_response));
	   if(ret == NULL) {
		   printf("Error allocating tracker_response ptr\n");
		   return 0;
	   }
	   ret->size = datasize;
	   ret->data = data;
	   printf("ret->data: %s\n", ret->data);
	   return ret;
   }
   else if(strncmp(tmp, "HTTP/1.0 200 OK", strlen("HTTP/1.0 200 OK")) == 0) {

	   len = recv(sockfd,recvline,2,0);
	   if(len <= 0) {
		   perror("Error, cannot read socket from tracker");
		   exit(-6);
	   }

	   datasize = MAXLINE;
	   int i, j = 0, m = 0, count_peers = 0;//记录tracker返回的peers数量 
	   char buf[datasize];
	   data = (char*)malloc(MAXLINE);
	   bool peer = false, peers_ip = false;
	   char num_peers[10], *ip_port;
	   
	   len = recv(sockfd, buf, datasize, 0);
	   if(len < 0) {
		   perror("Error, cannot read socket from tracker\n");
		   exit(-6);
	   }
	
	   for(i = 0; i < datasize; i++) {
	  
		   if(peer == true) {
			   if(buf[i] != ':') { 
				   num_peers[m] = buf[i];
				   m++;
			   }
			   else {
				   count_peers = atoi(num_peers)/6;
				   printf("count_peers: %d\n", count_peers);
				   peers_ip = true;
				   data[j] = 'l';
				   j++;
				   continue;
			   }
		   }
		   else {
			   data[j] = buf[i];
			   j++;
		   }
		   if(peers_ip == true) {
			   //printf("count_peers: %d\n", count_peers);
			   if(count_peers > 0) {
				   ip_port = &data[j];
				   char ip[16];
				   int ip_len, port;
				   unsigned int *p = (unsigned int *)&buf[i+4];
				   port = ntohs(*p);//通过指针计算端口号

				   struct in_addr ip_s;//提取ip地址
				   ip_s.s_addr = (*(int *)(&buf[i]));
				   printf("s_addr: %s\n", inet_ntoa(ip_s));
				   ip_len = strlen(inet_ntoa(ip_s));
				   sprintf(ip_port, "d7:peer id1:a2:ip%d:%s4:porti%dee", ip_len, inet_ntoa(ip_s), port);//id写成了单个字母a
				   i += 5;//切记不是加6
				   j += strlen(ip_port);
				   count_peers--;//列表中的项数
			   }
		   }
		   else if(i > 4) {
			   if(buf[i] == 's' && buf[i-1] == 'r' && buf[i-2] == 'e' && buf[i-3] == 'e' && buf[i-4] == 'p') {
				   peer = true;
			   }
		   }
	   }
	   data[j] = 'e'; data[j + 1] = 'e';
	   datasize = j + 2;
	   printf("datasize: %d\n", datasize);
	   data[datasize] = '\0';
	   for(i=0; i < datasize; i++)
		   printf("%c",data[i]);
	   printf("\n");

	   // 分配, 填充并返回tracker_response结构
	   tracker_response* ret;
	   ret = (tracker_response*)malloc(sizeof(tracker_response));
	   if(ret == NULL) {
		   printf("Error allocating tracker_response ptr\n");
		   return 0;
	   }
	   ret->size = datasize;
	   ret->data = data;
	   printf("ret->data: %s\n", ret->data);
	   return ret;

   }
   else {
	   perror("Error, didn't match HTTP line\n");
	   exit(-6);
   }

}
// 解码B编码的数据, 将解码后的数据放入tracker_data结构
tracker_data* get_tracker_data(char* data, int len)
{
  tracker_data* ret;
  be_node* ben_res;
  printf("data: %s\n", data);
  printf("len: %d\n", len);
  ben_res = be_decoden(data, len);
  printf("be_decoden\n");
  printf("len: %d\n", len);
  printf("BE_DICT: %d\n", BE_DICT);
  printf("type: %d\n", ben_res->type);//type总出问题,往往是信息没有正确读取
  if(ben_res->type != BE_DICT)
  {
    perror("Data not of type dict");
    exit(-12);
  }
  ret = (tracker_data*)malloc(sizeof(tracker_data));
  if(ret == NULL)
  {
    perror("Could not allcoate tracker_data");
    exit(-12);
  }
  // 遍历键并测试它们
  int i;
  for (i = 0; ben_res->val.d[i].val != NULL; i++)
  { 
    // 检查是否有失败键	
	printf("key: %s\n", ben_res->val.d[i].key);

    if(strncmp(ben_res->val.d[i].key, "failure reason", strlen("failure reason")) == 0)
    {
      printf("Error: %s",ben_res->val.d[i].val->val.s);
      exit(-12);
    }
	printf("======interval======\n");
    // interval键
    if(strncmp(ben_res->val.d[i].key,"interval",strlen("interval")) == 0)
    {
      ret->interval = (int)ben_res->val.d[i].val->val.i;
	  printf("interval: %d\n", ret->interval);
    }
    // peers键
	printf("======peers======\n");
    if(strncmp(ben_res->val.d[i].key,"peers",strlen("peers")) == 0)
    { 
      be_node* peer_list = ben_res->val.d[i].val;
      get_peers(ret, peer_list);
    }
  }
  be_free(ben_res);
  return ret;
}
// 处理来自Tracker的字典模式的peer列表
void get_peers(tracker_data* td, be_node* peer_list)
{
  int i;
  int numpeers = 0;
  // 计算列表中的peer数
  for (i=0; peer_list->val.l[i] != NULL; i++)
  {
	  printf("peer_list->val.l[i]->type: %d\n", peer_list->val.l[i]->type);
    // 确认元素是一个字典
    if(peer_list->val.l[i]->type != BE_DICT)
    {
      perror("Expecting dict, got something else");
	  break;
	  exit(-12);
    }

    // 找到一个peer, 增加numpeers
    numpeers++;
  }
  //printf("Num peers: %d\n",numpeers);
  // 为peer分配空间
  td->numpeers = numpeers;
  td->peers = (peerdata*)malloc(numpeers*sizeof(peerdata));
  if(td->peers == NULL)
  {
    perror("Couldn't allocate peers");
    exit(-12);
  }
  // 获取每个peer的数据

  for (i=0; peer_list->val.l[i] != NULL && i < numpeers; i++)
  {
    get_peer_data(&(td->peers[i]), peer_list->val.l[i]);
  }
  return;
}
// 给出一个peerdata的指针和一个peer的字典数据, 填充peerdata结构
void get_peer_data(peerdata* peer, be_node* ben_res)
{
  int i;
  if(ben_res->type != BE_DICT)
  {
    perror("Don't have a dict for this peer");
    exit(-12);
  }
  // 遍历键并填充peerdata结构
  for (i=0; ben_res->val.d[i].val != NULL; i++)
  { 
    //printf("%s\n",ben_res->val.d[i].key);
    // peer id键
    if(strncmp(ben_res->val.d[i].key,"peer id",strlen("peer id")) == 0)
    {
      //printf("Peer id: %s\n", ben_res->val.d[i].val->val.s);
      memcpy(peer->id,ben_res->val.d[i].val->val.s,20);
      peer->id[20] = '\0';
      /*
      int idl;
      printf("Peer id: ");
      for(idl=0; idl<len; idl++)
        printf("%02X ",(unsigned char)peer->id[idl]);
      printf("\n");
      */
    }
    // ip键
    if(strncmp(ben_res->val.d[i].key,"ip",strlen("ip")) == 0)
    {
      int len;
      //printf("Peer ip: %s\n",ben_res->val.d[i].val->val.s);
      len = strlen(ben_res->val.d[i].val->val.s);
      peer->ip = (char*)malloc((len+1)*sizeof(char));
      strcpy(peer->ip,ben_res->val.d[i].val->val.s);
    }
    // port键
    if(strncmp(ben_res->val.d[i].key,"port",strlen("port")) == 0)
    {
      //printf("Peer port: %d\n",ben_res->val.d[i].val->val.i);
      peer->port = ben_res->val.d[i].val->val.i;
    }
  }
}
