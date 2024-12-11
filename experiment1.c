#include"sys/types.h"
#include"sys/file.h"
#include"unistd.h"

char r_buf[4];//读缓冲
char w_buf[4];//写缓冲
int pipe_fd[2];
pid_t pid1, pid2, pid3, pid4;

int producer(int id);
int consumer(int id);

int main(int argc, char **argv) {
	if (pipe(pipe_fd) < 0) {//生成通信管道
		printf("pipe create error \n");
		exit(-1);
	}
	else {
		printf("pipe is created successfully! \n");
		if ((pid1 = fork()) == 0)producer(1);//生成生产者1
		if ((pid2 = fork()) == 0)producer(2); //生成生产者2
		if ((pid3 = fork()) == 0)consumer(1);//生成消费者1
		if ((pid4 = fork()) == 0)consumer(2);//生成消费者2
		//复制自己生成四个子进程
	}
	close(pipe_fd[0]);//关闭写入端口
	close(pipe_fd[1]);//关闭读取端口
	//防止一直占用读写端口导致别生产者或消费者程序一直等待，整个程序将无法结束退出


	int i, pid, status;
	for (i = 0; i < 4; i++) {
		pid = wait(&status);//等四个子进程结束后自己才可以结束,如果没有则原进程可能会比子进程早结束，因为原进程将不会等待子进程结束
	exit(0);
}

int producer(int id) {
	printf("producer %d is running! \n", id);
	close(pipe_fd[0]);//关闭读口，自己无需使用故不必占用
	int i = 0;
	for (i = 1; i < 10; i++) {//九次循环
		sleep(3);//每3秒执行一次循环内内容，每三秒执行一次写入操作
		if (id == 1)strcpy(w_buf, "aaa\0");//生产者1向写缓冲写入aaa
		else strcpy(w_buf,"bbb\0");//生产者2向写缓冲写入bbb
		if (write(pipe_fd[1], w_buf, 4) == -1)printf("write to pipe error\n");//进行从写缓冲写入通信管道四个字节操作，如果写入失败则输出错误
	}
	close(pipe_fd[1]);//关闭写口，自己无需使用故不必占用
	printf("producer %d is over!\n", id);//生产者进程结束
	exit(id);//退出生产者子进程防止子进程继续向下执行
}

int consumer(int id) {
	close(pipe_fd[1]);
	printf("producer %d is running! \n", id);
	if (id == 1)strcpy(w_buf, "ccc\0");//消费者1从读缓冲读取ccc
	else strcpy(w_buf,"ddd\0");//消费者2从读缓冲读取ddd
	while (1) {
		sleep(1);//每1秒执行一次循环内内容，用于等待管道另一头的写入信息每秒钟检查一次
		strcpy(r_buf, "eee\0");//将都缓冲填满eee
		if (read(pipe_fd[0], r_buf, 4) == 0)//正常成功读取则继续，读取为空则退出循环
			break;
		printf("consumer %d get %s,while the w_buf is %s\n", id, r_buf, w_buf);//输出读取内容
	}
	close(pipe_fd[0]);//关闭写口，自己无需使用故不必占用
	printf("consumer %d is over! \n", id);//消费者进程结束
	exit(id);//退出消费者子进程防止子进程继续向下执行
}