#include "math.h"
#include "sched.h"
#include "pthread.h"
#include "stdlib.h"
#include "semaphore.h"
#include "stdio.h"

typedef struct {
    char task_id;//�̱߳��
    int call_num;//���ô���
    int ci;//ִ������ʱ��
    int ti;//ˢ������
    int ci_left;//ʣ��ִ������ʱ��
    int ti_left;//������һ��ˢ�µ�ʱ��
    int flag;//�Ƿ��Ծ��־λ
    int arg;//��������

    int wait;//�ȴ�ʱ��

    pthread_t th;//��Ӧ�߳�
}task;

void proc(int* args);//ִ��������
void* idle();//�еȺ���
int select_proc();//ѡ��һ���߳̽��е���

int task_num = 0;//
int idle_num = 0;//

int alg;//ѡ����㷨��1 EDF�� 2 RMS��3 FCFS
int curr_proc = -1;//����ִ�е�������
int demo_time = 100;//Ĭ��ϵͳִ��ʱ��Ϊ100

task* tasks;
pthread_mutex_t proc_wait[100];//Ϊÿ�����񴴽�һ��������
pthread_mutex_t main_wait, idle_wait;//���������еȺ���������
float sum = 0;
pthread_t idle_proc;

int main(int argc, char** argv)
{
    pthread_mutex_init(&main_wait, NULL);//��ʼ��������main_wait
    pthread_mutex_lock(&main_wait);// �´�ִ��lock�ȴ�
    pthread_mutex_init(&idle_wait, NULL);//��ʼ��������idle_wait
    pthread_mutex_lock(&idle_wait);// �´�ִ��lock�ȴ�
    printf("Please input number of real time tasks:\n");
    scanf("%d", &task_num);
    tasks = (task*)malloc(task_num * sizeof(task));//Ϊ���������ռ�
    int i;
    for (i = 0; i < task_num; i++)
    {
        pthread_mutex_init(&proc_wait[i], NULL);//��ʼ��������task_num��������
        pthread_mutex_lock(&proc_wait[i]);
    }
    for (i = 0; i < task_num; i++)
    {
        printf("Please input task id, followed by Ci and Ti:");
        getchar();
        scanf("%c,%d,%d,", &tasks[i].task_id, &tasks[i].ci, &tasks[i].ti);
        tasks[i].ci_left = tasks[i].ci;
        tasks[i].ti_left = tasks[i].ti;
        tasks[i].flag = 2;
        tasks[i].arg = i;
        tasks[i].call_num = 1;
        sum = sum + (float)tasks[i].ci / (float)tasks[i].ti;//��sum�������ж��ܷ����
    }
    printf("Please input algorithm, 1 for EDF, 2 for RMS, 3 for FCFS:");
    getchar();
    scanf("%d", &alg);
    printf("Please input demo time:");
    scanf("%d", &demo_time);
    //��r�ж��Ƿ�����ɵ��ȵĳ������
    double r = 1;//EDF������
    if (alg == 2)
    {
        r = ((double)task_num) * (exp(log(2) / (double)task_num) - 1);//RMS������
        printf("r is %lf\n", r);
    }
    if (sum > r)//���ɵ���
    {
        printf("(sum=%lf>r=%lf), not schedulable!\n", sum, r);
        exit(2);
    }
    pthread_create(&idle_proc, NULL, (void*)idle, NULL);//�����й���̣�idle_proc�洢�й���̱�ʶ��
    for (i = 0; i < task_num; i++)
        pthread_create(&tasks[i].th, NULL, (void*)proc, &tasks[i].arg);//���������������
    
    for (i = 0; i < demo_time; i++)//ϵͳ��ʼִ�У�ÿ��ѭ���ʹ���һ��ʱ�䵥λ
    {
        int j;
        if ((curr_proc = select_proc(alg)) != -1)//�õ����㷨ѡ��һ���߳̿�ʼִ�У��������ֵΪ-1�����û�н��̿���ִ��
        {
            pthread_mutex_unlock(&proc_wait[curr_proc]);//���ѱ�ѡ�е��߳�
            pthread_mutex_lock(&main_wait);//�������ȴ�
        }
        else//����ֵΪ-1����û�н��̿���ִ�У�ѡ���й����ִ��
        {
            pthread_mutex_unlock(&idle_wait); //�����й��߳�
            pthread_mutex_lock(&main_wait);//�������ȴ�
        }
        //ִ��һ��ʱ��Ƭ��ֹͣ���ص���������������

        for (j = 0; j < task_num; j++)
        {
            tasks[j].wait++;// //�����������񣬽�ÿ������ĵȴ�ʱ��wait++
            if (--tasks[j].ti_left == 0) //�����������񣬽�ÿ�������ʣ��ʱ��ti_left-1
            {//��ti_left==0����ô˵����ǰ��������ڽ�������ʼ��һ������
                tasks[j].ti_left = tasks[j].ti;
                tasks[j].ci_left = tasks[j].ci;
                //��ti_left��ci_left����Ϊ��ʼֵ
                pthread_create(&tasks[j].th, NULL, (void*)proc, &tasks[j].arg);//����һ���µ��߳�
                tasks[j].flag = 2;//ͬʱ�������־flag��Ϊ2����ʾ��Ծ
                tasks[j].wait = 0;//waitʱ������
            }
        }
    }
    printf("\n");
    sleep(10);
};

void proc(int* args)
{//�����ʣ��ִ��ʱ��ci_left>0�����
    while (tasks[*args].ci_left > 0)
    {
        pthread_mutex_lock(&proc_wait[*args]); //������Ӧ�̵߳Ļ���������ִͣ�У�ֱ�������̻߳���
        if (idle_num != 0)
        {
            printf("idle(%d)", idle_num);//�����ʱ���й��̣߳���ô��ӡ�й��߳�����
            idle_num = 0;
        }
        printf("%c%d", tasks[*args].task_id, tasks[*args].call_num);//��ӡ�����ʶ��task_id�����ô���call_num
        tasks[*args].ci_left--; //ִ��һ��ʱ�䵥λ
        if (tasks[*args].ci_left == 0)//����ʣ��ʱ���Ϊ0����ʾִ�����
        {
            printf("(%d)", tasks[*args].ci);//��ӡ���ִ��ci��ʱ��Ƭ
            tasks[*args].flag = 0;//���ٻ�Ծ
            tasks[*args].call_num++;//���ӵ��ô���
        }
        pthread_mutex_unlock(&main_wait);//�������߳�
    }
};

void* idle()
{
    while (1)
    {
        pthread_mutex_lock(&idle_wait); //�ȴ�������
        printf("-> ");//��ӡ��ͷ��ʾ�պ�һ��ʱ�䵥λ
        idle_num++;//�й��̱߳�ִ�еĴ���+1
        pthread_mutex_unlock(&main_wait);//�������߳�
    }
};

int select_proc(int alg)//ѡ���̵߳����㷨
{
    int j;
    int temp1, temp2;
    temp1 = 10000;
    temp2 = -1;


    if ((alg==2)&&(curr_proc!=-1)&&(tasks[curr_proc].flag)!=0)//��ʾRMS�Ƿ���ռʽ
    return curr_proc;//���ص�ǰ����ִ�е��̺߳�
  
    for (j = 0; j < task_num; j++)//������������
    {
        if (tasks[j].flag == 2)//���ڻ�Ծ����flag==2�����ж�
        {
            switch (alg)//ѡ������㷨����
            {
            case 1://EDF
                
                if (temp1 > tasks[j].ti_left)//EDF�㷨Ѱ��ʣ��ʱ�����ٵ�
                {
                    temp1 = tasks[j].ti_left;
                    temp2 = j;
                }
                break;
            case 2://RMS
                if (temp1 > tasks[j].ti)//RMS�㷨������ti��С��
                {
                    temp1 = tasks[j].ti;
                    temp2 = j;
                }
                break;
            }
            case 3://FCFS
                if (temp1 < tasks[j].wait)//FCFS�㷨Ѱ�ҵȴ�ʱ����ļ����絽���
                {
                    temp1 = tasks[j].wait;
                    temp2 = j;
                }
                break;
        }
        }
    }
    return temp2;
}