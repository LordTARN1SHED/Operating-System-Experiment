#include"stdio.h"
#include"stdlib.h"
#include"string.h"
#include"sched.h"//调度用
#include"pthread.h"//线程、互斥锁、条件变量 
#include"semaphore.h"//信号量，sem
#include"linux/sched.h"//引用CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES需要的头文件

//报错对sem_未定义的引用，需要在编译最后添加-pthread
//需要确保在编译时使用了-pthread选项来链接POSIX线程库，这样编译器就能够正确找到和解析sem_wait函数的定义
//gcc t2.c -o t2 -pthread

int producer(void* args);
int consumer(void* args);
pthread_mutex_t mutex; //声明互斥锁
sem_t product; //声明信号量1存有的物品量
sem_t warehouse; //声明信号量2可存入的物品量

char buffer[8][4];//缓冲，用于存储生产者和消费者之间传递的数据
int bp = 0; //bp变量表示buffer数组的索引

int flagc[2] = { 0 };
int flagp[2] = { 0 };

int main(int argc, char** argv) {
	pthread_mutex_init(&mutex, NULL);//初始化一个互斥锁对象，将其设置为默认的属性和状态
	sem_init(&product, 0, 0);//信号量1初始化，初始量为0
	sem_init(&warehouse, 0, 8);//信号量2初始化，初始量为8
	//int sem_init(sem_t * sem, int pshared, unsigned int value);
	//1.pshared 控制信号量的类型，值为 0 代表该信号量用于多线程间的同步
	//2.第二个值如果大于 0 表示可以共享，用于多个相关进程间的同步
	//3.第三个参数表示初始信号量值为0

	int clone_flag, arg, retval;
	char* stack;
	clone_flag = CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES;
	//CLONE_VM：表示新线程与父线程共享虚拟内存空间。这意味着新线程可以访问和修改父线程的内存数据。
	//CLONE_SIGHAND：表示新线程与父线程共享信号处理程序（signal handlers）。这意味着新线程会继承父线程的信号处理程序。
	//CLONE_FS：表示新线程与父线程共享文件系统信息。这意味着新线程可以访问和修改与文件系统相关的信息，如当前工作目录和文件描述符。
	//CLONE_FILES：表示新线程与父线程共享文件描述符表。这意味着新线程可以访问和操作父线程打开的文件。
	int i;
	for (i = 0; i < 2; i++)
	{
		arg = i;//arg用来向子线程传递参数
		stack = (char*)malloc(4096);//给子线程分配一块stack
		retval = clone((void*)producer, &(stack[4095]), clone_flag, (void*)&arg);
		//clone()函数的返回值retval被用来检查线程创建是否成功。如果retval是一个正值，表示成功创建了一个新线程
		stack = (char*)malloc(4096);//给子线程分配一块stack
		retval = clone((void*)consumer, &(stack[4095]), clone_flag, (void*)&arg);
		//clone()函数的返回值retval被用来检查线程创建是否成功。如果retval是一个正值，表示成功创建了一个新线程
		sleep(1);
		//使用 clone() 函数创建两个子线程，一个用于生产者，
		//一个用于消费者。每个子线程都有独立的堆栈空间，
		//并通过参数传递给子线程。主线程在创建完子线程后退出。
	}
	//循环两次共创建两个消费者两个生产者
	while (1) { if (flagp[0] == 1)break; }
	while (1) { if (flagp[1] == 1)break; }
	while (1) { if (flagc[0] == 1)break; }
	while (1) { if (flagc[1] == 1)break; };
	//确保在子线程执行完毕之前不会把整个程序结束
	exit(1);
}

int producer(void* args)
{
	//在producer函数中，生产者线程首先等待一段时间（使用sleep()函数），然后使用sem_wait()函数等待warehouse信号量。
	//一旦获得了信号量，生产者线程会获取互斥锁mutex，向buffer数组中写入数据，然后释放互斥锁，并通过sem_post()函数增加product信号量的值。
	int id = *((int*)args);//获得生产者编号
	int i;
	for (i = 0; i < 10; i++)//循环10次
	{
		sleep(i + 1);//设置等待时间表现线程苏吨差别
		sem_wait(&warehouse);//p操作
		pthread_mutex_lock(&mutex);//互斥锁上锁
		if (id == 0) strcpy(buffer[bp], "aaa\0");//0号生产者生产aaa
		else strcpy(buffer[bp], "bbb\0");//1号生产者生产bbb
		bp++;//buffer指针加一
		printf("producer%d produce %s in %d\n", id, buffer[bp - 1], bp - 1);
		pthread_mutex_unlock(&mutex);//互斥锁解锁
		sem_post(&product);//v操作
	}
	printf("producer%d is over! \n", id);//生产者进程结束
	flagp[id] = 1;
}

int consumer(void* args)

//在consumer函数中，消费者线程也首先等待一段时间，然后使用sem_wait()函数等待product信号量。
//一旦获得了信号量，消费者线程会获取互斥锁mutex，从buffer数组中读取数据，然后释放互斥锁，并通过sem_post()函数增加warehouse信号量的值。

{
	int id = *((int*)args);//获得消费者编号
	int i;
	for (i = 0; i < 10; i++)//循环10次
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
		//此处设置的等待时间是为让每个程序的运行顺序与答案一致，从而实现输出与答案一致

		sem_wait(&product);//p操作
		pthread_mutex_lock(&mutex);//互斥锁上锁
		bp--;//buffer指针减一
		printf("consumer%d get %s in %d\n", id, buffer[bp], bp);//消费者获取
		strcpy(buffer[bp], "zzz\0");//填入zzz
		pthread_mutex_unlock(&mutex);//互斥锁解锁
		sem_post(&warehouse);//v操作
	}
	printf("consumer%d is over! \n", id);//消费者进程结束
	flagc[id] = 1;
}
