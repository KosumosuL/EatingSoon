#include <deque>
#include <pthread.h>

class LucasThreadPool
{
public:
	typedef struct 
	{
    	void (*function)(void *);
    	void *argument;
	}Task;
public:
	LucasThreadPool(int l_thread_cnt_ = 10);
	~LucasThreadPool();

	int 			add(void (*function)(void *), void *argument);
	int 			stop();
	Task 			start();

private:
	volatile bool 		l_run;
	volatile int 		l_thread_cnt;
	pthread_t*			l_threads;
	std::deque<Task> 	l_tasks;
	pthread_mutex_t 	l_mutex;
	pthread_cond_t 		l_cond;	
private:
	int 			createThread();
	static void* 	threadRoutine(void* arg);
};