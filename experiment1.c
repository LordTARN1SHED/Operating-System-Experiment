#include"sys/types.h"
#include"sys/file.h"
#include"unistd.h"

char r_buf[4];//������
char w_buf[4];//д����
int pipe_fd[2];
pid_t pid1, pid2, pid3, pid4;

int producer(int id);
int consumer(int id);

int main(int argc, char **argv) {
	if (pipe(pipe_fd) < 0) {//����ͨ�Źܵ�
		printf("pipe create error \n");
		exit(-1);
	}
	else {
		printf("pipe is created successfully! \n");
		if ((pid1 = fork()) == 0)producer(1);//����������1
		if ((pid2 = fork()) == 0)producer(2); //����������2
		if ((pid3 = fork()) == 0)consumer(1);//����������1
		if ((pid4 = fork()) == 0)consumer(2);//����������2
		//�����Լ������ĸ��ӽ���
	}
	close(pipe_fd[0]);//�ر�д��˿�
	close(pipe_fd[1]);//�رն�ȡ�˿�
	//��ֹһֱռ�ö�д�˿ڵ��±������߻������߳���һֱ�ȴ������������޷������˳�


	int i, pid, status;
	for (i = 0; i < 4; i++) {
		pid = wait(&status);//���ĸ��ӽ��̽������Լ��ſ��Խ���,���û����ԭ���̿��ܻ���ӽ������������Ϊԭ���̽�����ȴ��ӽ��̽���
	exit(0);
}

int producer(int id) {
	printf("producer %d is running! \n", id);
	close(pipe_fd[0]);//�رն��ڣ��Լ�����ʹ�ùʲ���ռ��
	int i = 0;
	for (i = 1; i < 10; i++) {//�Ŵ�ѭ��
		sleep(3);//ÿ3��ִ��һ��ѭ�������ݣ�ÿ����ִ��һ��д�����
		if (id == 1)strcpy(w_buf, "aaa\0");//������1��д����д��aaa
		else strcpy(w_buf,"bbb\0");//������2��д����д��bbb
		if (write(pipe_fd[1], w_buf, 4) == -1)printf("write to pipe error\n");//���д�д����д��ͨ�Źܵ��ĸ��ֽڲ��������д��ʧ�����������
	}
	close(pipe_fd[1]);//�ر�д�ڣ��Լ�����ʹ�ùʲ���ռ��
	printf("producer %d is over!\n", id);//�����߽��̽���
	exit(id);//�˳��������ӽ��̷�ֹ�ӽ��̼�������ִ��
}

int consumer(int id) {
	close(pipe_fd[1]);
	printf("producer %d is running! \n", id);
	if (id == 1)strcpy(w_buf, "ccc\0");//������1�Ӷ������ȡccc
	else strcpy(w_buf,"ddd\0");//������2�Ӷ������ȡddd
	while (1) {
		sleep(1);//ÿ1��ִ��һ��ѭ�������ݣ����ڵȴ��ܵ���һͷ��д����Ϣÿ���Ӽ��һ��
		strcpy(r_buf, "eee\0");//������������eee
		if (read(pipe_fd[0], r_buf, 4) == 0)//�����ɹ���ȡ���������ȡΪ�����˳�ѭ��
			break;
		printf("consumer %d get %s,while the w_buf is %s\n", id, r_buf, w_buf);//�����ȡ����
	}
	close(pipe_fd[0]);//�ر�д�ڣ��Լ�����ʹ�ùʲ���ռ��
	printf("consumer %d is over! \n", id);//�����߽��̽���
	exit(id);//�˳��������ӽ��̷�ֹ�ӽ��̼�������ִ��
}