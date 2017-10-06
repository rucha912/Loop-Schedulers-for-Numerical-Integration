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


float global_result = 0, global_x_int;
float a, b;
unsigned long n;
double cpu_time;
int func, intensity, granularity, work_done = 0, nbthreads;
char* sync;
unsigned long startloop=0, endloop;
float result = 0, global_x_val ;

pthread_mutex_t loop_locks, global_result_lock, iteration_lock;

struct arguments
{
    float a, b;
    unsigned long n, start, end;
    float result, x_val=0, x_int;
    int intensity, func;
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
    
void* integrate_iteration_level(void * argument)
{
    struct arguments* arg = (struct arguments* )argument;
    
    while (work_done!= 1)
    {
  		arg->start = get_start();
    	arg->end = get_end(arg->start);
    	for(int i = arg->start; i <= arg->end; i++)
    	{
    		pthread_mutex_lock(&global_result_lock);
			global_x_int = (arg->a + (i + 0.5) * ((arg->b - arg->a) / (float)arg->n));
			global_x_val = global_x_val + global_x_int;
			switch(arg->func)
    		{
	 
    			case 1: global_result =  f1(global_x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
				break;
    	      	case 2: global_result = f2(global_x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
				break;
    	      	case 3: global_result = f3(global_x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
				break;
    	  	  	case 4: global_result = f4(global_x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
				break;
    	    	default: std::cout<<"\nWrong function id"<<std::endl;
    	  	}
    	  	pthread_mutex_unlock(&global_result_lock);
    	  }
    }
    	  pthread_exit(NULL);
}

//This function does integration using chunk level mutual exclusion. The critical section is in the while loop for every computing thread.

void* integrate_chunk_level(void *unused)
{
	float chunk_result = 0, chunk_int, chunk_val=0;
	unsigned long loop_end, loop_start;
	while(work_done != 1)
	{
		loop_start = get_start();
		loop_end = get_end(loop_start);
		for(int i = loop_start; i <= loop_end; i++)
    	{	
			chunk_int = (a + (i + 0.5) * ((b - a) / (float)n));
			chunk_val = chunk_val + chunk_int;
			switch(func)
        	{
      			case 1: chunk_result = f1(chunk_val, intensity) * ((b - a)/n);
						break;
        		case 2: chunk_result = f2(chunk_val, intensity) * ((b - a)/n);
						break;
        	  	case 3: chunk_result = f3(chunk_val, intensity) * ((b - a)/n);
						break;
      		  	case 4: chunk_result = f4(chunk_val, intensity) * ((b - a)/n);
						break;
        	  	default: std::cout<<"\nWrong function id"<<std::endl;
      		}
		}	
	}
	pthread_mutex_lock(&global_result_lock);
    global_result = global_result + chunk_result;
    pthread_mutex_unlock(&global_result_lock);
}

//This function does integration using thread level mutual exclusion where the every thread computes the result in the local variable and aggregates the result in the end.

void* integrate_thread_level(void * argument)
{
    struct arguments* arg = (struct arguments* )argument;
    
    while(work_done!=1)
    {
    
    	arg->start = get_start();
    	arg->end = get_end(arg->start);
   
    	for(int i = arg->start; i <= arg->end; i++)
    	{
    	  
    		arg->x_int = (arg->a + (i + 0.5) * ((arg->b - arg->a) / (float)arg->n));
			arg->x_val = arg->x_val + arg->x_int;
			switch(arg->func)
    		{
      			case 1: arg->result = f1(arg->x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
	      		break;
          		case 2: arg->result = f2(arg->x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
	      		break;
          		case 3: arg->result = f3(arg->x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
	      		break;
      	  		case 4: arg->result = f4(arg->x_val, arg->intensity) * ((arg->b - arg->a)/arg->n);
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
	struct arguments* arg;
	
    threads = (pthread_t *)malloc(nbthreads * sizeof(pthread_t));
    arg = (struct arguments*)malloc(nbthreads * sizeof(arguments));
    
    auto clock_start = std::chrono::system_clock::now();
    
    
    if ( strcmp(sync, "thread") == 0)
    {
    	for ( int j = 0; j < nbthreads; j++)
    	{
    	    arg[j].a = a;
    	    arg[j].b = b;
    	    arg[j].intensity =intensity;
    	    arg[j].func = func;
    	    arg[j].n = n;
    	    pthread_create(&threads[j], NULL, integrate_thread_level, (void *)&(arg[j])); 
        }
	for ( int i = 0; i < nbthreads; i++)
        {
    	    pthread_join(threads[i], NULL);
    	}
    	for( int k = 0; k < nbthreads; k++)
    	{
    	    global_result += arg[k].result;
    	}
    }
    else if(strcmp(sync, "iteration") == 0)
    {
    	for ( int j = 0; j < nbthreads; j++)
    	{
    	    arg[j].a = a;
    	    arg[j].b = b;
    	    arg[j].intensity =intensity;
    	    arg[j].func = func;
    	    arg[j].n = n;
    	    pthread_create(&threads[j], NULL, integrate_iteration_level, (void *)&(arg[j])); 
        }
        
        for(int j = 0; j < nbthreads; j++)
        {
            pthread_join(threads[j], NULL);
        }
    }
 	else if( strcmp(sync, "chunk") == 0)
 	{
 		for(int i = 0; i < nbthreads; i++)
 		{
 			pthread_create(&threads[i], NULL, integrate_chunk_level, NULL);
 		}
 	
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
