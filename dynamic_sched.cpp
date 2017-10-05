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
int startloop=0, endloop;
float result = 0, x_val = 0;

pthread_mutex_t loop_locks, global_result_lock, iteration_lock;

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
    
void* integrate_iteration_level(void *unused)
{
	float loop_result;
	int loop_end, loop_start;
	pthread_t thread_id = pthread_self();
	while(work_done != 1)
	{
		loop_start = get_start();
		loop_end = get_end(loop_start);
		std::cout<<"Start:"<<loop_start<<"End:"<<loop_end<<std::endl;
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

void* integrate_chunk_level(void *unused)
{
	float chunk_result = 0;
	int loop_end, loop_start;
	while(work_done != 1)
	{
		loop_start = get_start();
		loop_end = get_end(loop_start);
		std::cout<<"Start:"<<loop_start<<"End:"<<loop_end<<std::endl;
		for(int i = loop_start; i < loop_end; i++)
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
		
	}
}

void* integrate_thread_level(void* unused)
{
	float thread;
	int loop_end, loop_start;
	while(work_done != 1)
	{
		loop_start = get_start();
		loop_end = get_end(loop_start);
		std::cout<<"Start:"<<loop_start<<"End:"<<loop_end<<std::endl;
	
		for(int i = loop_start; i < loop_end; i++)
    	{	
			x_int = (a + (i + 0.5) * ((b - a) / (float)n));
			x_val = x_val + x_int;
			switch(func)
        	{
      			case 1: thread += f1(x_val, intensity) * ((b - a)/n);
						break;
        		case 2: thread += f2(x_val, intensity) * ((b - a)/n);
						break;
        	  	case 3: thread += f3(x_val, intensity) * ((b - a)/n);
						break;
      		  	case 4: thread += f4(x_val, intensity) * ((b - a)/n);
						break;
        	  	default: std::cout<<"\nWrong function id"<<std::endl;
      		}
		}
	}
	pthread_exit((void *)&thread);
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
	float *thread_ans;
	
    threads = (pthread_t *)malloc(nbthreads * sizeof(pthread_t));
    thread_ans = (float *)malloc(nbthreads * sizeof(float));
    
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
 	if( strcmp(sync, "thread") == 0)
 	{
		for ( int i = 0; i < nbthreads; i++)
    	{
        	pthread_join(threads[i], (void **)&thread_ans[i]);
    	}
    	for(int i = 0; i < nbthreads; i++)
 		{
 			global_result += thread_ans[i];
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
    std::cout<<global_result<<std::endl;
    std::cerr<<diff.count()<<std::endl;
    

  return 0;
}
