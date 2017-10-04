#include <iostream>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<math.h>
#include<chrono>


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
float *thread_ans;
clock_t start, end;
float a, b;
unsigned long n;
double cpu_time;
int func, intensity, granularity, work_done = 0, nbthreads;
char* sync;
int startloop=0, endloop;
float result = 0, x_val = 0;

pthread_mutex_t loop_locks, global_result_lock, iteration_lock;

int get_end(int start)
{	
	pthread_mutex_lock(&loop_locks);
	endloop = (start + granularity) - 1;
	pthread_mutex_unlock(&loop_locks);
	if( endloop == n-1)
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
    
void* integrate_iteration_level(void *unused)
{
	float loop_result;
	int loop_end, loop_start;
	pthread_t thread_id = pthread_self();
	while(work_done != 1)
	{
		loop_start = get_start();
		loop_end = get_end(loop_start);
		std::cout<<"Thread ID:"<<thread_id<<"Start:"<<loop_start<<"End:"<<loop_end<<std::endl;
		for(int i = loop_start; i < loop_end; i++)
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
	
		if(endloop >= n)
		{
			pthread_mutex_lock(&loop_locks);
			work_done = 1;
			pthread_mutex_unlock(&loop_locks);
			pthread_exit(NULL);
		}
		else
		{
			pthread_mutex_lock(&loop_locks);
			endloop = loop_end;
			pthread_mutex_unlock(&loop_locks);
		}
	}
}

void* integrate_chunk_level(void *unused)
{
	float chunk_result = 0;
	while(work_done != 1)
	{
		pthread_mutex_lock(&loop_locks);
		endloop = get_end(startloop);
		pthread_mutex_unlock(&loop_locks);
		
		std::cout<<"Start:"<<startloop<<"End:"<<endloop<<std::endl;
		for(int i = startloop; i < endloop; i++)
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
      		pthread_mutex_lock(&global_result_lock);
      		global_result = chunk_result;
      		pthread_mutex_unlock(&global_result_lock);	
		}
		
		if(endloop == n-1)
		{
			work_done = 1;
			pthread_exit(NULL);
		}
		else
		{
			pthread_mutex_lock(&loop_locks);
			startloop = endloop;
			pthread_mutex_unlock(&loop_locks);
		}
	}
}

void* integrate_thread_level(void* unused)
{
	uintptr_t id = (uintptr_t)unused;
	while(work_done != 1)
	{
		pthread_mutex_lock(&loop_locks);
		endloop = get_end(startloop);
		pthread_mutex_unlock(&loop_locks);
	
		for(int i = startloop; i < endloop; i++)
    	{	
			x_int = (a + (i + 0.5) * ((b - a) / (float)n));
			x_val = x_val + x_int;
			switch(func)
        	{
        		std::cout<<"Index:"<<id<<std::endl;
      			case 1: thread_ans[id] += f1(x_val, intensity) * ((b - a)/n);
						break;
        		case 2: thread_ans[id] += f2(x_val, intensity) * ((b - a)/n);
						break;
        	  	case 3: thread_ans[id] = f3(x_val, intensity) * ((b - a)/n);
						break;
      		  	case 4: thread_ans[id] = f4(x_val, intensity) * ((b - a)/n);
						break;
        	  	default: std::cout<<"\nWrong function id"<<std::endl;
      		}
		}
		if(endloop == n)
		{
			work_done = 1;
			pthread_exit(NULL);
		}
		else
		{
			pthread_mutex_lock(&loop_locks);
			startloop = endloop;
			pthread_mutex_unlock(&loop_locks);
		}
	}
}

  
int main (int argc, char* argv[]) {

	func = atoi(argv[1]);
    a = atof(argv[2]);
    b = atof(argv[3]);
    n = atof(argv[4]);
    intensity = atoi(argv[5]);
    nbthreads = atoi(argv[6]);
    sync = argv[7];
	granularity = atoi(argv[8]);
    pthread_t threads[nbthreads];
    
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
 			pthread_create(&threads[i], NULL, integrate_thread_level, NULL);
 		}
 	}
 	else if( strcmp(sync, "chunk") == 0)
 	{
 		for(int i = 0; i < nbthreads; i++)
 		{
 			pthread_create(&threads[i], NULL, integrate_chunk_level, NULL);
 		}
 	}
	for ( int i = 0; i < nbthreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    if( strcmp(sync, "thread") == 0)
    {
 		for(int i = 0; i < nbthreads; i++)
 		{
 			global_result += thread_ans[i];
 		}
 	}
 	
 	auto clock_end = std::chrono::system_clock::now();
    std::chrono::duration<double>diff = clock_end - clock_start;
    std::cout<<global_result<<std::endl;
    std::cerr<<diff.count()<<std::endl;
    

  return 0;
}
