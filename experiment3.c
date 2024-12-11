#include "math.h"
#include "sched.h"
#include "pthread.h"
#include "stdlib.h"
#include "semaphore.h"
#include "stdio.h"

typedef struct {
    char task_id;//线程编号
    int call_num;//调用次数
    int ci;//执行所需时间
    int ti;//刷新周期
    int ci_left;//剩余执行所需时间
    int ti_left;//距离下一次刷新的时间
    int flag;//是否活跃标志位
    int arg;//调度类型

    int wait;//等待时间

    pthread_t th;//对应线程
}task;

void proc(int* args);//执行任务函数
void* idle();//闲等函数
int select_proc();//选择一个线程进行调度

int task_num = 0;//
int idle_num = 0;//

int alg;//选择的算法，1 EDF， 2 RMS，3 FCFS
int curr_proc = -1;//正在执行的任务编号
int demo_time = 100;//默认系统执行时间为100

task* tasks;
pthread_mutex_t proc_wait[100];//为每个任务创建一个互斥锁
pthread_mutex_t main_wait, idle_wait;//主函数，闲等函数互斥锁
float sum = 0;
pthread_t idle_proc;

int main(int argc, char** argv)
{
    pthread_mutex_init(&main_wait, NULL);//初始化并锁定main_wait
    pthread_mutex_lock(&main_wait);// 下次执行lock等待
    pthread_mutex_init(&idle_wait, NULL);//初始化并锁定idle_wait
    pthread_mutex_lock(&idle_wait);// 下次执行lock等待
    printf("Please input number of real time tasks:\n");
    scanf("%d", &task_num);
    tasks = (task*)malloc(task_num * sizeof(task));//为任务声明空间
    int i;
    for (i = 0; i < task_num; i++)
    {
        pthread_mutex_init(&proc_wait[i], NULL);//初始化并锁定task_num个互斥锁
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
        sum = sum + (float)tasks[i].ci / (float)tasks[i].ti;//求sum，用来判断能否调度
    }
    printf("Please input algorithm, 1 for EDF, 2 for RMS, 3 for FCFS:");
    getchar();
    scanf("%d", &alg);
    printf("Please input demo time:");
    scanf("%d", &demo_time);
    //用r判断是否满足可调度的充分条件
    double r = 1;//EDF的条件
    if (alg == 2)
    {
        r = ((double)task_num) * (exp(log(2) / (double)task_num) - 1);//RMS的条件
        printf("r is %lf\n", r);
    }
    if (sum > r)//不可调度
    {
        printf("(sum=%lf>r=%lf), not schedulable!\n", sum, r);
        exit(2);
    }
    pthread_create(&idle_proc, NULL, (void*)idle, NULL);//创建闲逛进程，idle_proc存储闲逛进程标识符
    for (i = 0; i < task_num; i++)
        pthread_create(&tasks[i].th, NULL, (void*)proc, &tasks[i].arg);//创建所有任务进程
    
    for (i = 0; i < demo_time; i++)//系统开始执行，每个循环就代表一个时间单位
    {
        int j;
        if ((curr_proc = select_proc(alg)) != -1)//用调度算法选择一个线程开始执行，如果返回值为-1则代表没有进程可以执行
        {
            pthread_mutex_unlock(&proc_wait[curr_proc]);//唤醒被选中的线程
            pthread_mutex_lock(&main_wait);//主函数等待
        }
        else//返回值为-1代表没有进程可以执行，选择闲逛进程执行
        {
            pthread_mutex_unlock(&idle_wait); //唤醒闲逛线程
            pthread_mutex_lock(&main_wait);//主函数等待
        }
        //执行一个时间片后停止，回到主函数继续运行

        for (j = 0; j < task_num; j++)
        {
            tasks[j].wait++;// //遍历所有任务，将每个任务的等待时间wait++
            if (--tasks[j].ti_left == 0) //遍历所有任务，将每个任务的剩余时间ti_left-1
            {//若ti_left==0，那么说明当前任务的周期结束，开始下一个周期
                tasks[j].ti_left = tasks[j].ti;
                tasks[j].ci_left = tasks[j].ci;
                //将ti_left和ci_left重置为初始值
                pthread_create(&tasks[j].th, NULL, (void*)proc, &tasks[j].arg);//创建一个新的线程
                tasks[j].flag = 2;//同时将任务标志flag设为2，表示活跃
                tasks[j].wait = 0;//wait时间清零
            }
        }
    }
    printf("\n");
    sleep(10);
};

void proc(int* args)
{//任务的剩余执行时间ci_left>0则继续
    while (tasks[*args].ci_left > 0)
    {
        pthread_mutex_lock(&proc_wait[*args]); //锁定对应线程的互斥锁，暂停执行，直到被主线程唤醒
        if (idle_num != 0)
        {
            printf("idle(%d)", idle_num);//如果此时有闲逛线程，那么打印闲逛线程数量
            idle_num = 0;
        }
        printf("%c%d", tasks[*args].task_id, tasks[*args].call_num);//打印任务标识符task_id，调用次数call_num
        tasks[*args].ci_left--; //执行一个时间单位
        if (tasks[*args].ci_left == 0)//任务剩余时间变为0，表示执行完毕
        {
            printf("(%d)", tasks[*args].ci);//打印完成执行ci个时间片
            tasks[*args].flag = 0;//不再活跃
            tasks[*args].call_num++;//增加调用次数
        }
        pthread_mutex_unlock(&main_wait);//唤醒主线程
    }
};

void* idle()
{
    while (1)
    {
        pthread_mutex_lock(&idle_wait); //等待被调度
        printf("-> ");//打印箭头表示空耗一个时间单位
        idle_num++;//闲逛线程被执行的次数+1
        pthread_mutex_unlock(&main_wait);//唤醒主线程
    }
};

int select_proc(int alg)//选择线程调度算法
{
    int j;
    int temp1, temp2;
    temp1 = 10000;
    temp2 = -1;


    if ((alg==2)&&(curr_proc!=-1)&&(tasks[curr_proc].flag)!=0)//表示RMS是非抢占式
    return curr_proc;//返回当前正在执行的线程号
  
    for (j = 0; j < task_num; j++)//遍历所有任务
    {
        if (tasks[j].flag == 2)//对于活跃任务flag==2进行判断
        {
            switch (alg)//选择调度算法种类
            {
            case 1://EDF
                
                if (temp1 > tasks[j].ti_left)//EDF算法寻找剩余时间最少的
                {
                    temp1 = tasks[j].ti_left;
                    temp2 = j;
                }
                break;
            case 2://RMS
                if (temp1 > tasks[j].ti)//RMS算法找周期ti最小的
                {
                    temp1 = tasks[j].ti;
                    temp2 = j;
                }
                break;
            }
            case 3://FCFS
                if (temp1 < tasks[j].wait)//FCFS算法寻找等待时间最长的即最早到达的
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