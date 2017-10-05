#include<iostream>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<math.h>
#include<chrono>
#include<stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

float f1(float x, int intensity);
float f2(float x, int intensity);
float f3(float x, int intensity);
float f4(float x, int intensity);

#ifdef __cplusplus
}
#endif


float global_result = 0, x_int;
clock_t start, end;
float a, b;
unsigned long n;
double cpu_time;
int func, intensity, granularity, work_done = 0, nbthreads;
char* sync;
unsigned long startloop=0, endloop;
float result = 0, x_val = 0;

pthread_mutex_t loop_locks, global_result_lock, iteration_lock;

struct thread_arg
{
	float thread_result, x_val=0, x_int;
	unsigned long start, end;
};

int get_end(int start)
{	
	pthread_mutex_lock(&loop_locks);
	endloop = (start + granularity) - 1;
	pthread_mutex_unlock(&loop_locks);
	if( endloop == n - 1)
		work_done = 1;
	return endloop;
}

int get_start()
{
	int temp;
	temp = startloop;
	pthread_mutex_lock(&loop_locks);
	startloop = startloop + granularity;
	pthread_mutex_unlock(&loop_locks);
	return temp;
}

//This function does the integration using iteration level mutual exclusion. The critical section is in the innermost loop.
    
void* integrate_iteration_level(void *unused)
{
	float loop_result;
	unsigned long loop_end, loop_start;
	pthread_t thread_id = pthread_self();
	while(work_done != 1)
	{
		loop_start = get_start();
		loop_end = get_end(loop_start);
		for(int i = loop_start; i <= loop_end; i++)
    	{	
    		pthread_mutex_lock(&iteration_lock);
			x_int = (a + (i + 0.5) * ((b - a) / (float)n));
			x_val = x_val + x_int;
			switch(func)
        	{
      			case 1: global_result = f1(x_val, intensity) * ((b - a)/n);
						break;
        		case 2: global_result = f2(x_val, intensity) * ((b - a)/n);
						break;
        	  	case 3: global_result = f3(x_val, intensity) * ((b - a)/n);
						break;
      		  	case 4: global_result = f4(x_val, intensity) * ((b - a)/n);
						break;
        	  	default: std::cout<<"\nWrong function id"<<std::endl;
      		}
      		pthread_mutex_unlock(&iteration_lock);	
		}
	}
}

//This function does integration using chunk level mutual exclusion. The critical section is in the while loop for every computing thread.

void* integrate_chunk_level(void *unused)
{
	float chunk_result = 0;
	unsigned long loop_end, loop_start;
	while(work_done != 1)
	{
		loop_start = get_start();
		loop_end = get_end(loop_start);
		for(int i = loop_start; i <= loop_end; i++)
    	{	
			x_int = (a + (i + 0.5) * ((b - a) / (float)n));
			x_val = x_val + x_int;
			switch(func)
        	{
      			case 1: chunk_result = f1(x_val, intensity) * ((b - a)/n);
						break;
        		case 2: chunk_result = f2(x_val, intensity) * ((b - a)/n);
						break;
        	  	case 3: chunk_result = f3(x_val, intensity) * ((b - a)/n);
						break;
      		  	case 4: chunk_result = f4(x_val, intensity) * ((b - a)/n);
						break;
        	  	default: std::cout<<"\nWrong function id"<<std::endl;
      		}
		}
		pthread_mutex_lock(&global_result_lock);
      	global_result = chunk_result;
      	pthread_mutex_unlock(&global_result_lock);	
		
	}
}

//This function does integration using thread level mutual exclusion where the every thread computes the result in the local variable and aggregates the result in the end.

void* integrate_thread_level(void* thread_args)
{
	float thread;
	struct thread_arg* arg = (struct thread_arg *)thread_args;
	while(work_done != 1)
	{
		arg->start = get_start();
		arg->end = get_end(arg->start);

		for(int i = arg->start; i <= arg->end; i++)
    	{	
			arg->x_int = (a + (i + 0.5) * ((b - a) / (float)n));
			arg->x_val = arg->x_val + arg->x_int;
			switch(func)
        	{
      			case 1: arg->thread_result = f1(arg->x_val, intensity) * ((b - a)/n);
						break;
        		case 2: arg->thread_result = f2(arg->x_val, intensity) * ((b - a)/n);
						break;
        	  	case 3: arg->thread_result = f3(arg->x_val, intensity) * ((b - a)/n);
						break;
      		  	case 4: arg->thread_result = f4(arg->x_val, intensity) * ((b - a)/n);
						break;
        	  	default: std::cout<<"\nWrong function id"<<std::endl;
      		}
		}
	}
	pthread_exit(NULL);
}

  
int main (int argc, char* argv[])
{

	func = atoi(argv[1]);
    a = atof(argv[2]);
    b = atof(argv[3]);
    n = atof(argv[4]);
    intensity = atoi(argv[5]);
    nbthreads = atoi(argv[6]);
    sync = argv[7];
	granularity = atoi(argv[8]);
    pthread_t *threads;
	struct thread_arg* arg;
	
    threads = (pthread_t *)malloc(nbthreads * sizeof(pthread_t));
    arg = (struct thread_arg*)malloc(nbthreads * sizeof(thread_arg));
    
    auto clock_start = std::chrono::system_clock::now();
    
    if( strcmp(sync, "iteration") == 0)
    {
 		for(int i = 0; i < nbthreads; i++)
 		{
 			pthread_create(&threads[i], NULL, integrate_iteration_level, NULL);
 		}
 	}
 	
    else if( strcmp(sync, "thread") == 0)
    {
 		for(int i = 0; i < nbthreads; i++)
 		{
 			pthread_create(&threads[i], NULL, integrate_thread_level, (void *)&(arg[i]));
 		}
 	}
 	else if( strcmp(sync, "chunk") == 0)
 	{
 		for(int i = 0; i < nbthreads; i++)
 		{
 			pthread_create(&threads[i], NULL, integrate_chunk_level, NULL);
 		}
 	}
 	if( strcmp(sync, "thread") == 0)
 	{
		for ( int i = 0; i < nbthreads; i++)
    	{
        	pthread_join(threads[i], NULL);
    	}
    	for( int k = 0; k < nbthreads; k++)
    	{
    	    global_result += arg[k].thread_result;
    	}
    }
    else
    {
    	for(int i = 0; i < nbthreads; i++)
 		{
 			pthread_join(threads[i], NULL);
 		}
 	}
 	
 	auto clock_end = std::chrono::system_clock::now();
    std::chrono::duration<double>diff = clock_end - clock_start;
    std::cout<<global_result;
    std::cerr<<diff.count();
    

  return 0;
}
