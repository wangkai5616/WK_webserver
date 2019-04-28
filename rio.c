//
// Latest edit by TeeKee on 2017/7/23.
//

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "rio.h"

//�˺������Դ�fd�ж�ȡn���ַ���usrbuf�У���read������ȣ�
//�����źŴ������жϺ���ٴγ��Զ�ȡ����ˣ��ڳ��˿ɶ��ַ���С��n����£�
//�ú������Ա�֤��ȡn���ֽڡ�
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char *)usrbuf;
    while(nleft > 0){
        if((nread = read(fd, bufp, nleft)) < 0){
			//���źŴ������ж�
			if(errno == EINTR)
                nread = 0;//���ζ���0���ַ����ٴζ�ȡ
            else
                return -1;
        }
		//��ȡ��EOF
        else if(nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
	//����ʵ�ʶ�ȡ���ֽ���
    return (n - nleft);
}

/*
rio_writen(int fd, void *, size_t)�����ļ��������Ĳ�����int���ͣ�������rio_t���͡�
��Ϊrio_writen����Ҫд���塣����Ϊʲô�أ���������˵����Ȼ����Ϊread��װ��rio_readn�ṩ�˻�������
Ϊʲô��ҲΪwrite�ṩһ���л����rio_writen�����أ�

����һ��������������дһ��http�������ģ�Ȼ���������д���˶�Ӧsocket���ļ��������Ļ�������
���軺������СΪ8K���������Ĵ�СΪ1K����ô�����������������Ϊ�������Ż��Զ�����
����д���ļ�������һ��Ҳ���������ģ����Ǿ���˵���û���ṩһ��ˢ�»������ĺ����ֶ�ˢ�£�
�һ���Ҫ���ⷢ��7K�����ݽ���������������������Ĳ���������д�뵽socket���С�
���ԣ�һ����л������ĺ����ⶼ��һ��ˢ�»������ĺ��������ڽ��ڻ���������������д���ļ����У�
��ʹ������û�б�����������Ҳ��C��׼���������Ȼ�������һ������Աһ��С��������д��������
���ֶ�ˢ�£���ô�����ݣ������ģ���һֱפ���ڻ�����������Ľ��̻���ɵɵ�صȴ���Ӧ��
*/
//�˺���ͬ����֤д��n�ֽڡ�
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char *)usrbuf;

    while(nleft > 0){
        if((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)
                nwritten = 0;
            else{
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
//���л�������RIO���������������ڵ�Ŀ����Ϊ�˼������ε���ϵͳ��IO������
//�����ں�̬�������Ķ��⿪����
/*
�ײ��ȡ�����������������ݳ���ʱ���˺���ֱ�ӿ�����
���������ݸ��ϲ��ȡ������������������ʱ���ú���ͨ��ϵͳ����
���ļ��ж�ȡ����������ֽڵ����������ٿ������������ݸ��ϲ㺯��
*/
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    size_t cnt;
	//��������û������ʱ����ȥִ��ϵͳ����
    while(rp->rio_cnt <= 0){
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0){
            if(errno == EAGAIN){
                return -EAGAIN;
            }
			//�����ж����͵Ĵ���Ļ�Ӧ�ý��ж�ȡ�����򷵻ش���
            if(errno != EINTR){
                return -1;
            }
        }
		//��ȡ����EOF
        else if(rp->rio_cnt == 0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }
    cnt = n;
    if(rp->rio_cnt < (ssize_t)n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

//�˺�������rio_readn,������Ϊ�����˻����������Լ����������ں�̬ʱ�Ŀ�����
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char *)usrbuf;
    while (nleft > 0){
		//���ε���static����rio_read
        if((nread = rio_read(rp, bufp, nleft)) < 0){
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        else if(nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}

//��ȡһ�е����ݣ�����'\n'��β����һ��
//������rio_t�����û���ַ��Ŀ�ĵ�ַ��һ�е���󳤶�
//�����ֽ����������з���
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    size_t n;
    ssize_t rc;
    char c, *bufp = (char *)usrbuf;
    for(n = 1; n < maxlen; n++){
        if((rc = rio_read(rp, &c, 1)) == 1){
            *bufp++ = c;
            if(c == '\n')
            break;
        }
		//û�н��յ�����
        else if(rc == 0){
			//�����һ��ѭ����û���յ����ݣ�����������ݿɽ���
            if (n == 1){
                return 0;
            }
            else
                break;
        }
        else if(rc == -EAGAIN){
            return rc;
        }
        else{
            return -1;
        }
    }
    *bufp = 0;
    return n;
}
