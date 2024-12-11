
//hashfile.c
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<memory.h>
#include"HashFile.h"

int hashfile_creat(const char* filename, mode_t mode, int reclen, int total_rec_num)
{
	struct HashFileHeader hfh;//���ڴ洢�ļ���ͷ����Ϣ
	int fd;//���ڴ洢�ļ�������
	int rtn;//���ڴ洢д���ļ��ķ���ֵ
	char* buf;//���ڷ����¼�ռ�Ļ�����
	int i = 0;
	hfh.sig = 31415926;//�����ļ���ӡ��������У���ļ�����ȷ��
	hfh.reclen = reclen;//���ü�¼�ĳ���
	hfh.total_rec_num = total_rec_num;//�����ܼ�¼��
	hfh.current_rec_num = 0;// ����ǰ��¼����ʼ��Ϊ 0
	//fd = open(filename,mode);
	fd = creat(filename, mode);//���� creat ���������ļ����������ص��ļ��������洢�� fd ��
	if (fd != -1)
	{//����ļ������ɹ�����д���ļ�ͷ����Ϣ���ļ���
		rtn = write(fd, &hfh, sizeof(struct HashFileHeader));
		//lseek(fd,sizeof(struct HashFileHeader),SEEK_SET);
		if (rtn != -1)
		{//���ͷ����Ϣд��ɹ���������¼�ռ�Ļ����� buf
			buf = (char*)malloc((reclen + sizeof(struct CFTag)) * total_rec_num);
			memset(buf, 0, (reclen + sizeof(struct CFTag)) * total_rec_num);//����¼�ռ�Ļ�������ʼ��Ϊ��
			rtn = write(fd, buf, (reclen + sizeof(struct CFTag)) * total_rec_num);//��������ʼ���ļ�¼�ռ�д���ļ�
			free(buf);//�ͷ�֮ǰ������ڴ�ռ�
		}
		close(fd);//�ر��ļ�
		return rtn;//����д���ļ��ķ���ֵ
	}
	else {
		close(fd);
		return -1;
	}
}

int hashfile_open(const char* filename, int flags, mode_t mode)
//�ú�����Ŀ���Ǵ�һ���Ѵ��ڵĹ�ϣ�ļ�������֤�ļ��������ԣ��Ա�����Ĺ�ϣ�ļ�������������Ч���ļ��Ͻ���
{
	int fd = open(filename, flags, mode);//��ָ���ļ��������ص��ļ��������洢�� fd ��
	struct HashFileHeader hfh;
	if (read(fd, &hfh, sizeof(struct HashFileHeader)) != -1)//ʹ�� read �������ļ��ж�ȡͷ����Ϣ��������洢�ڱ��� hfh ��
	{
		lseek(fd, 0, SEEK_SET);//�ļ�ָ������Ϊ�ļ���ͷ
		if (hfh.sig == 31415926)//���ӡ��ֵ����Ԥ���ֵ��31415926�����򷵻��ļ������� fd����ʾ�ļ��򿪳ɹ�
			return fd;
		else
			return -1;
	}
	else return -1;
}

int hashfile_close(int fd)
{
	return close(fd);//����close�������ر��ļ�������Ϊfd���ļ�
}

int hashfile_read(int fd, int keyoffset, int keylen, void* buf)
{
	struct HashFileHeader hfh;
	readHashFileHeader(fd, &hfh);//���ļ�ͷ����Ϣ��ȡ������ hfh ��
	int offset = hashfile_findrec(fd, keyoffset, keylen, buf);
	//���ݼ�ֵ�ڹ�ϣ�ļ��в��Ҽ�¼��ƫ��������������洢�ڱ��� offset ��
	if (offset != -1)
	{
		lseek(fd, offset + sizeof(struct CFTag), SEEK_SET);//�ļ�ָ�붨λ����¼��λ�ã����������Ϣ
		return read(fd, buf, hfh.reclen);//�ӹ�ϣ�ļ��ж�ȡ��¼�����ݣ�������洢�ڻ����� buf ��
	}
	else
	{//δ�ҵ���Ч��¼
		return -1;
	}
}

int hashfile_write(int fd, int keyoffset, int keylen, void* buf)
{
	return hashfile_saverec(fd, keyoffset, keylen, buf);
	//return -1;
}

int hashfile_delrec(int fd, int keyoffset, int keylen, void* buf)
{
	int offset;
	offset = hashfile_findrec(fd, keyoffset, keylen, buf);//���Ҿ���ָ����ֵ��Ҫɾ�����ļ�¼��������¼��ƫ�����洢�� offset ������
	if (offset != -1)//�ҵ���ƥ��ļ�¼
	{
		struct CFTag tag;
		read(fd, &tag, sizeof(struct CFTag));//���ļ��ж�ȡ�� offset ��Ӧ�ļ�¼�ı�ǩ��Ϣ��������洢�� tag ������
		tag.free = 0; //����¼�Ŀ��б�־��Ϊ 0����ʾ�ü�¼�����ǿ���״̬ 
		lseek(fd, offset, SEEK_SET);//���ļ�ָ���ƶ��� offset ��Ӧ��λ�ã���Ҫɾ���ļ�¼��λ��
		write(fd, &tag, sizeof(struct CFTag));//�����º�ı�ǩ��Ϣд���ļ�����ʾ�ü�¼�����ǿ���״̬
		struct HashFileHeader hfh;
		readHashFileHeader(fd, &hfh);//��ȡ��ϣ�ļ���ͷ��Ϣ��������洢�� hfh ������
		int addr = hash(keyoffset, keylen, buf, hfh.total_rec_num);//����ָ����ֵ�ڹ�ϣ���еĵ�ַ
		offset = sizeof(struct HashFileHeader) + addr * (hfh.reclen + sizeof(struct CFTag));//����ָ����ֵ�ļ�¼���ļ��е�ƫ����
		if (lseek(fd, offset, SEEK_SET) == -1)//���ļ�ָ���ƶ�����¼��ƫ�������ڵ�λ��
			return -1;
		read(fd, &tag, sizeof(struct CFTag));//��ȡ���¼ƫ������Ӧ�ı�ǩ��Ϣ��������洢�� tag ������
		tag.collision--; //��ͻ������1����ʾ������һ����ͻ
		lseek(fd, offset, SEEK_SET);// ���ļ�ָ���ƶ�����¼ƫ�������ڵ�λ��
		write(fd, &tag, sizeof(struct CFTag));// �����º�ı�ǩ��Ϣд���ļ�
		hfh.current_rec_num--; //��ǰ��¼����1
		lseek(fd, 0, SEEK_SET);//���ļ�ָ���ƶ�����ϣ�ļ��Ŀ�ͷ
		write(fd, &hfh, sizeof(struct HashFileHeader));//�����º�Ĺ�ϣ�ļ�ͷ��Ϣд���ļ�
	}
	else {
		return -1;
	}
}

int hashfile_findrec(int fd, int keyoffset, int keylen, void* buf)
{
	struct HashFileHeader hfh;
	readHashFileHeader(fd, &hfh);//��ȡHash�ļ���ͷ��Ϣ��hfh�ṹ�����
	int addr = hash(keyoffset, keylen, buf, hfh.total_rec_num);//���������ֵ�ڹ�ϣ���еĵ�ַ
	int offset = sizeof(struct HashFileHeader) + addr * (hfh.reclen + sizeof(struct CFTag));//�����¼���ļ��е�ƫ����
	if (lseek(fd, offset, SEEK_SET) == -1)//���ļ�ָ�붨λ����¼��λ��
		return -1;
	struct CFTag tag;
	read(fd, &tag, sizeof(struct CFTag));//��ȡ��¼��CFTag��Ϣ��tag�ṹ�����
	char count = tag.collision;//����ͻ�������浽count
	if (count == 0)//�����ͻ����Ϊ0����ʾ�ü�¼�����ڣ���������-1
		return -1; //������
recfree://��ǩ��������ת����¼�ͷŵĲ���
	if (tag.free == 0)//��鵱ǰ��¼�Ƿ�Ϊ����״̬
	{//���Ϊ��
		offset += hfh.reclen + sizeof(struct CFTag);//����ƫ������ָ����һ����¼��λ��
		if (lseek(fd, offset, SEEK_SET) == -1)//���ļ�ָ�붨λ����һ����¼����ʼλ��
			return -1;
		read(fd, &tag, sizeof(struct CFTag));//��ȡ��һ����¼��CFTag��Ϣ��tag�ṹ�����
		goto recfree;
	}
	else {//����ǿ�
		char* p = (char*)malloc(hfh.reclen * sizeof(char));//����һ��������p�����ڴ洢��ȡ�ļ�¼����
		read(fd, p, hfh.reclen);//��ȡ��¼���ݵ�������p
		//printf("Record is {%d , %s}\n",((struct jtRecord *)p)->key,((struct jtRecord *p)->other);
		char* p1, * p2;
		p1 = (char*)buf + keyoffset;
		p2 = p + keyoffset;
		//��ʼ������ָ�룬�ֱ�ָ�������ֵ�Ͷ�ȡ�ļ�¼�����ж�Ӧλ�õ��ַ�
		int j = 0;//��ʼ��������j
		while ((*p1 == *p2) && (j < keylen))
		{//ѭ���Ƚ�����ָ����ָ���ַ��Ƿ���ͬ����δ�ﵽ������ֵ����
			p1++;
			p2++;
			j++;
		}
		if (j == keylen)
		{
			free(p);//�ͷŻ�����p���ڴ�
			p = NULL;
			return(offset);//�������ҵ��ļ�¼��ƫ����
		}
		else {
			if (addr == hash(keyoffset, keylen, p, hfh.total_rec_num))//��鵱ǰ��¼�ĵ�ַ�Ƿ������¼���ĵ�ַ��ͬ
			{//����ͬ
				count--;//��ͻ������1
				if (count == 0)//����Ƿ��Ϊ0
				{//����Ϊ0����ʾ������ù�ϣͰ������Ԫ��
					free(p);//�ͷŻ�����p���ڴ�
					p = NULL;
					return -1;//����-1����ʾ�����ڣ���ʾδ�ҵ�ƥ��ļ�¼
				}
			}
			//����ͬ
			free(p);//�ͷŻ�����p���ڴ�
			p = NULL;
			offset += hfh.reclen + sizeof(struct CFTag);//����ƫ������ָ����һ����¼��λ��
			if (lseek(fd, offset, SEEK_SET) == -1)//���ļ�ָ�붨λ����һ����¼����ʼλ��
				return -1;
			read(fd, &tag, sizeof(struct CFTag));//��ȡ��һ����¼��CFTag��Ϣ��tag�ṹ�����
			goto recfree;//��ת��recfree��ǩ�����������м�¼�ͷŵ��ж�
		}
	}
}

int hashfile_saverec(int fd, int keyoffset, int keylen, void* buf)
{
	if (checkHashFileFull(fd))
	{//�����ϣ�ļ��������򷵻� -1����ʾд������޷�ִ��
		return -1;
	}
	struct HashFileHeader hfh;
	readHashFileHeader(fd, &hfh);//���ļ��ж�ȡͷ����Ϣ��������洢�� hfh ������
	int addr = hash(keyoffset, keylen, buf, hfh.total_rec_num);
	//���� hash ���������ݼ�ֵ��ƫ���������Ⱥ��ܼ�¼�������ϣ��ַ
	int offset = sizeof(struct HashFileHeader) + addr * (hfh.reclen + sizeof(struct CFTag));
	//���ݹ�ϣ��ַ�����¼���ļ��е�ƫ����
	if (lseek(fd, offset, SEEK_SET) == -1)//���ļ�ָ���ƶ�����¼��ƫ������
		return -1;
	struct CFTag tag;
	read(fd, &tag, sizeof(struct CFTag));//���ļ��ж�ȡ��ǰ��¼�ĳ�ͻ���
	tag.collision++;//����ͻ������1����ʾ�����˳�ͻ
	lseek(fd, sizeof(struct CFTag) * (-1), SEEK_CUR);//���ļ�ָ����ݵ���ͻ��ǵ�λ��
	write(fd, &tag, sizeof(struct CFTag));//���³�ͻ����
	while (tag.free != 0) 
	{//ʹ��˳��̽��ķ�ʽ�����ͻ������������һ����¼��ֱ���ҵ����еļ�¼λ��
		offset += hfh.reclen + sizeof(struct CFTag);//��ƫ��������һ����¼�ĳ��Ⱥ�һ����ͻ��ǵĳ���
		if (offset >= lseek(fd, 0, SEEK_END))//����Ƿ񳬹����ļ�ĩβ
			offset = sizeof(struct HashFileHeader); //����������ļ�ĩβ����ƫ�������ݵ��ļ�����ʼλ��
		if (lseek(fd, offset, SEEK_SET) == -1)//�����ļ�ָ��λ��
			return -1;
		read(fd, &tag, sizeof(struct CFTag));//���¶�ȡ��λ�õĳ�ͻ��־
	}
	//����ҵ��Ŀ��м�¼
	tag.free = -1;//���ҵ��Ŀ��м�¼�ı�־����Ϊ -1����ʾ�ѱ�ռ��
	lseek(fd, sizeof(struct CFTag) * (-1), SEEK_CUR);//���ݵ���ͻ��ǵ�λ��
	write(fd, &tag, sizeof(struct CFTag));//���¼�¼�ĳ�ͻ���
	write(fd, buf, hfh.reclen);//����¼����д���ļ�
	hfh.current_rec_num++;//����ǰ��¼����1����ʾд����һ���¼�¼
	lseek(fd, 0, SEEK_SET);//���ļ�ָ���ƶ����ļ�����ʼλ��
	return write(fd, &hfh, sizeof(struct HashFileHeader));  //�����º���ļ�ͷ����Ϣд���ļ���������д���� 
}

int hash(int keyoffset, int keylen, void* buf, int total_rec_num)
{
	int i = 0;
	char* p = (char*)buf + keyoffset;
	//����һ���ַ���ָ�� p����ָ���¼�������йؼ��ֵ���ʼλ�á�
	//ͨ���� buf ת��Ϊ�ַ���ָ�룬Ȼ����� keyoffset ��ƫ�������õ���ָ��ؼ��ֵ�ָ��
	int addr = 0;//���ڴ洢����õ��Ĺ�ϣ��ַ
	for (i = 0; i < keylen; i++)//���ؼ��ֵ�ÿ���ַ�������ֵ���
	{
		addr += (int)(*p);
		p++;
	}
	return addr % (int)(total_rec_num * COLLISIONFACTOR);
	//����õ��Ĺ�ϣ��ַ addr �����ܼ�¼�����Գ�ͻ���� (COLLISIONFACTOR)��Ȼ��ȡ�������õ����յĹ�ϣ��ַ
}

int checkHashFileFull(int fd)
{
	struct HashFileHeader hfh;
	readHashFileHeader(fd, &hfh);//���ļ������� fd ��ָ��Ĺ�ϣ�ļ���ͷ����Ϣ��ȡ�� hfh ������
	if (hfh.current_rec_num < hfh.total_rec_num)
		return 0;// �����ϣ�ļ�δ�������� 0����ʾ��ϣ�ļ�δ��
	else
		return 1;//�����ϣ�ļ����������� 1����ʾ��ϣ�ļ�����
}

int readHashFileHeader(int fd, struct HashFileHeader* hfh)
{
	lseek(fd, 0, SEEK_SET);//�������ļ�ָ�붨λ���ļ��Ŀ�ͷ��ȷ�����ļ��Ŀ�ͷ��ʼ��ȡͷ����Ϣ
	return read(fd, hfh, sizeof(struct HashFileHeader));
	//�ֽڵ����ݣ�������ȡ�����ݴ洢�� hfh ָ��Ľṹ������С������ķ���ֵ��ʵ�ʶ�ȡ���ֽ���
}
