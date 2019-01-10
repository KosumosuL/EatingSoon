/* http线程池部分
 * 将线程池部分封装成了一个类，实例化线程池类后，将每个任务需要的函数和参数
 * 组合成一个task对象加入到任务队列中，每次空闲线程取出一个task并运行。
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "lucasthread.h"

LucasThreadPool::LucasThreadPool(int l_thread_cnt_)
{
	l_run = true;
	l_thread_cnt = l_thread_cnt_;
	createThread();
}

LucasThreadPool::~LucasThreadPool()
{
	stop();
}

/**********************************************************************/
/* 完成mutex和condition的初始化
 * 创建l_thread_cnt个线程*/
/**********************************************************************/
int LucasThreadPool::createThread()
{
	pthread_mutex_init(&l_mutex, NULL);
	pthread_cond_init(&l_cond, NULL);
	l_threads = (pthread_t*)malloc(sizeof(pthread_t)* l_thread_cnt);
	for(int i=0;i<l_thread_cnt;i++)
		pthread_create(&l_threads[i], NULL, threadRoutine, this);
	return 0;
}

/**********************************************************************/
/* 完成线程池的终止和清除 */
/**********************************************************************/
int LucasThreadPool::stop()
{
	if(!l_run)	return 1;
	l_run = false;
	pthread_cond_broadcast(&l_cond);

	for(int i=0;i<l_thread_cnt;i++)
		pthread_join(l_threads[i], NULL);
	free(l_threads);
	l_threads = NULL;
	l_tasks.clear();

	pthread_mutex_destroy(&l_mutex);
	pthread_cond_destroy(&l_cond);
	return 0;
}

/**********************************************************************/
/* 根据function和argument创建task结构体，加入任务队列 */
/**********************************************************************/
int LucasThreadPool::add(void (*function)(void *), void *argument)
{
	if(function == NULL)	return -1;
	pthread_mutex_lock(&l_mutex);
	Task task;
	task.function = function;
	task.argument = argument;
	l_tasks.push_back(task);
	pthread_mutex_unlock(&l_mutex);
	pthread_cond_signal(&l_cond);
	return 0;
}

/**********************************************************************/
/* 在线程池不为空、正在运行且任务队列不为空的情况下取出头任务 */
/**********************************************************************/
LucasThreadPool::Task LucasThreadPool::start()
{
	Task task;
	pthread_mutex_lock(&l_mutex);
	while(l_tasks.empty() && l_run)
		pthread_cond_wait(&l_cond, &l_mutex);

	if(!l_run)
	{
		pthread_mutex_unlock(&l_mutex);
		return task;
	}

	task = l_tasks.front();
	l_tasks.pop_front();
	pthread_mutex_unlock(&l_mutex);
	return task;
}

/**********************************************************************/
/* 执行死循环不断取出任务并完成相应任务 */
/**********************************************************************/
void* LucasThreadPool::threadRoutine(void* arg)
{
	LucasThreadPool* threadPool = static_cast<LucasThreadPool*>(arg);
	while(threadPool->l_run)
	{
		LucasThreadPool::Task task = threadPool->start();
		if(!task.function || !task.argument)
		{
			// printf("exit one thread\n");
			break;
		}
		(*(task.function))(task.argument);
	}
	return 0;
}