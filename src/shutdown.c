#include "util.h"

#include "pwpsignal.h"
extern int g_done;

// ��ȷ�Ĺرտͻ���
void client_shutdown(int sig)
{
  // ����ȫ��ֹͣ������ֹͣ���ӵ�����peer, �Լ���������peer������. Set global stop variable so that we stop trying to connect to peers and
  // �����������peer���ӵ��׽��ֺ����ӵ�����peer���߳�.
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
