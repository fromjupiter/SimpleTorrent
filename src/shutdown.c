#include "util.h"

#include "pwpsignal.h"
extern int g_done;

// 正确的关闭客户端
void client_shutdown(int sig)
{
  // 设置全局停止变量以停止连接到其他peer, 以及允许其他peer的连接. Set global stop variable so that we stop trying to connect to peers and
  // 这控制了其他peer连接的套接字和连接到其他peer的线程.
  g_done = 0;
  int mlen;
  char *MESG = make_tracker_request(BT_STOPPED, &mlen);
  printf("STOP MESG: %s\n", MESG);
  send(sockfd_traker, MESG, mlen, 0);

  close(sockfd_traker);
	int i=0;
	while(peers_head!=NULL){
		close(peers_head->sockfd);
		peer_t *temp=peers_head;
		peers_head=peers_head->next;
		free(temp);
	}	
}
