/*************************************************************************
	> File Name: file_check.c
	> Author: liucheng
	> Mail: 1035377294@qq.com 
	> Created Time: Sun 01 Jun 2014 09:20:07 PM CST
 ************************************************************************/

#include<stdio.h>
#include "util.h"
#include "sha1.h"
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
bool SHA_COMP(unsigned *sha_value,char* p){
    int count=0;
    int i;
    for(i=0;i<5;i++){
        unsigned info=sha_value[i];
        char *sub_info=(char *)&info;
        int j;
        for(j=0;j<4;j++){
            if(p[count++]!=sub_info[j])
                return false;
        }
    }
    return true;
}
//将文件分片读取，判断sha值，初始化bitfield_local
void check_file(char *name){
	bitfield_local=(char *)malloc(g_num_pieces);
    int i=0;
    //if(strcmp(name,"*.temp"))
    //  return; 
	printf("name :    %s  numpiece :  %d   piece_len : %d\n",name,g_num_pieces,g_torrentmeta->piece_len);
    FILE *f=fopen(name,"r");
    if(f==NULL){
        file_init(name);
        f=fopen(name,"r");
    }
    char buf[g_torrentmeta->piece_len];
    for(i=0;i<g_num_pieces;i++){
		unsigned sha_value[5];
        fseek(f,i*(g_torrentmeta->piece_len),SEEK_SET);
		int length=0;
		if(g_filelen-i*g_torrentmeta->piece_len>g_torrentmeta->piece_len){
			length=g_torrentmeta->piece_len;
			fread(buf,sizeof(char),g_torrentmeta->piece_len,f);
		}
		else{
			length=g_filelen-i*g_torrentmeta->piece_len;
			fread(buf,sizeof(char),g_filelen-i*g_torrentmeta->piece_len,f);
		}
		//printf("buf:  %s\n",buf);
		printf("%d   !!!!!!!!!!!\n",i);
        SHA1Context sha;
        SHA1Reset(&sha) ;
        SHA1Input(&sha,buf,length);
        if(!SHA1Result(&sha)){
            fprintf(stderr,"ERROR-- could not compute meassage digest\n");
        }
        else{
            printf("\t");
            int j;
            for(j=0;j<5;j++){
                sha_value[j]=htonl(sha.Message_Digest[j]);
				printf("%X ",sha.Message_Digest[j]);	
            }
        }
		int k=0;
		char* p=SHA1_piece(i);
        if(SHA_COMP(sha_value,p)){//<<<<<<<<<<<<<<<<<<<<<<
			piece_num_current++;
            bitfield_local[i]=1;
        }
        else{
            bitfield_local[i]=0;
        }
		printf("!!!!!!!!!!!\n");
    }
	printf("bitfield_local(%d):  ",g_num_pieces);
	for(i=0;i<g_num_pieces;i++){
		printf("%d ",bitfield_local[i]);
	}
	printf("\n");
    fclose(f);
}
//初始化文件，如果文件为空则产生临时文件，否则判定文件sha值
int file_init(char *name){
    FILE *f_temp=fopen(name,"w");
    printf("-----------!!!!!!!!\n");
    //char a='0';
    ftruncate(fileno(f_temp),g_filelen);
    //fwrite(&a,sizeof(char),g_filelen,f_temp);
    fclose(f_temp);
    printf("!!!!!!!!-----\n");
    int i=0;
    for(;i<g_num_pieces;i++)
        bitfield_local[i]=0;
    printf("!!!-----!!!!!\n");
    return 0;
}
