#include"stdio.h"
#include"stdlib.h"
#include"string.h"
#include"sched.h"//������
#include"pthread.h"//�̡߳����������������� 
#include"semaphore.h"//�ź�����sem
#include"linux/sched.h"//����CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES��Ҫ��ͷ�ļ�

//�����sem_δ��������ã���Ҫ�ڱ���������-pthread
//��Ҫȷ���ڱ���ʱʹ����-pthreadѡ��������POSIX�߳̿⣬�������������ܹ���ȷ�ҵ��ͽ���sem_wait�����Ķ���
//gcc t2.c -o t2 -pthread

int producer(void* args);
int consumer(void* args);
pthread_mutex_t mutex; //����������
sem_t product; //�����ź���1���е���Ʒ��
sem_t warehouse; //�����ź���2�ɴ������Ʒ��

char buffer[8][4];//���壬���ڴ洢�����ߺ�������֮�䴫�ݵ�����
int bp = 0; //bp������ʾbuffer���������

int flagc[2] = { 0 };
int flagp[2] = { 0 };

int main(int argc, char** argv) {
	pthread_mutex_init(&mutex, NULL);//��ʼ��һ�����������󣬽�������ΪĬ�ϵ����Ժ�״̬
	sem_init(&product, 0, 0);//�ź���1��ʼ������ʼ��Ϊ0
	sem_init(&warehouse, 0, 8);//�ź���2��ʼ������ʼ��Ϊ8
	//int sem_init(sem_t * sem, int pshared, unsigned int value);
	//1.pshared �����ź��������ͣ�ֵΪ 0 ������ź������ڶ��̼߳��ͬ��
	//2.�ڶ���ֵ������� 0 ��ʾ���Թ������ڶ����ؽ��̼��ͬ��
	//3.������������ʾ��ʼ�ź���ֵΪ0

	int clone_flag, arg, retval;
	char* stack;
	clone_flag = CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES;
	//CLONE_VM����ʾ���߳��븸�̹߳��������ڴ�ռ䡣����ζ�����߳̿��Է��ʺ��޸ĸ��̵߳��ڴ����ݡ�
	//CLONE_SIGHAND����ʾ���߳��븸�̹߳����źŴ������signal handlers��������ζ�����̻߳�̳и��̵߳��źŴ������
	//CLONE_FS����ʾ���߳��븸�̹߳����ļ�ϵͳ��Ϣ������ζ�����߳̿��Է��ʺ��޸����ļ�ϵͳ��ص���Ϣ���統ǰ����Ŀ¼���ļ���������
	//CLONE_FILES����ʾ���߳��븸�̹߳����ļ�������������ζ�����߳̿��Է��ʺͲ������̴߳򿪵��ļ���
	int i;
	for (i = 0; i < 2; i++)
	{
		arg = i;//arg���������̴߳��ݲ���
		stack = (char*)malloc(4096);//�����̷߳���һ��stack
		retval = clone((void*)producer, &(stack[4095]), clone_flag, (void*)&arg);
		//clone()�����ķ���ֵretval����������̴߳����Ƿ�ɹ������retval��һ����ֵ����ʾ�ɹ�������һ�����߳�
		stack = (char*)malloc(4096);//�����̷߳���һ��stack
		retval = clone((void*)consumer, &(stack[4095]), clone_flag, (void*)&arg);
		//clone()�����ķ���ֵretval����������̴߳����Ƿ�ɹ������retval��һ����ֵ����ʾ�ɹ�������һ�����߳�
		sleep(1);
		//ʹ�� clone() ���������������̣߳�һ�����������ߣ�
		//һ�����������ߡ�ÿ�����̶߳��ж����Ķ�ջ�ռ䣬
		//��ͨ���������ݸ����̡߳����߳��ڴ��������̺߳��˳���
	}
	//ѭ�����ι�������������������������
	while (1) { if (flagp[0] == 1)break; }
	while (1) { if (flagp[1] == 1)break; }
	while (1) { if (flagc[0] == 1)break; }
	while (1) { if (flagc[1] == 1)break; };
	//ȷ�������߳�ִ�����֮ǰ����������������
	exit(1);
}

int producer(void* args)
{
	//��producer�����У��������߳����ȵȴ�һ��ʱ�䣨ʹ��sleep()��������Ȼ��ʹ��sem_wait()�����ȴ�warehouse�ź�����
	//һ��������ź������������̻߳��ȡ������mutex����buffer������д�����ݣ�Ȼ���ͷŻ���������ͨ��sem_post()��������product�ź�����ֵ��
	int id = *((int*)args);//��������߱��
	int i;
	for (i = 0; i < 10; i++)//ѭ��10��
	{
		sleep(i + 1);//���õȴ�ʱ������߳��նֲ��
		sem_wait(&warehouse);//p����
		pthread_mutex_lock(&mutex);//����������
		if (id == 0) strcpy(buffer[bp], "aaa\0");//0������������aaa
		else strcpy(buffer[bp], "bbb\0");//1������������bbb
		bp++;//bufferָ���һ
		printf("producer%d produce %s in %d\n", id, buffer[bp - 1], bp - 1);
		pthread_mutex_unlock(&mutex);//����������
		sem_post(&product);//v����
	}
	printf("producer%d is over! \n", id);//�����߽��̽���
	flagp[id] = 1;
}

int consumer(void* args)

//��consumer�����У��������߳�Ҳ���ȵȴ�һ��ʱ�䣬Ȼ��ʹ��sem_wait()�����ȴ�product�ź�����
//һ��������ź������������̻߳��ȡ������mutex����buffer�����ж�ȡ���ݣ�Ȼ���ͷŻ���������ͨ��sem_post()��������warehouse�ź�����ֵ��

{
	int id = *((int*)args);//��������߱��
	int i;
	for (i = 0; i < 10; i++)//ѭ��10��
	{
		if (i < 4) {
			sleep(9 - i);
		}
		else if (i == 4) {
			sleep(8);
		}
		else if (i == 8) {
			sleep(0);
		}
		else if (i == 9) {
			sleep(4);
		}
		else {
			sleep(11 - i);
		}
		//�˴����õĵȴ�ʱ����Ϊ��ÿ�����������˳�����һ�£��Ӷ�ʵ��������һ��

		sem_wait(&product);//p����
		pthread_mutex_lock(&mutex);//����������
		bp--;//bufferָ���һ
		printf("consumer%d get %s in %d\n", id, buffer[bp], bp);//�����߻�ȡ
		strcpy(buffer[bp], "zzz\0");//����zzz
		pthread_mutex_unlock(&mutex);//����������
		sem_post(&warehouse);//v����
	}
	printf("consumer%d is over! \n", id);//�����߽��̽���
	flagc[id] = 1;
}
