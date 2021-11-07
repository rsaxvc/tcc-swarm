#define MULTITHREAD 1
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <libtcc.h>
#include <iostream>

static std::mutex Queue_Mutex;
static std::mutex threadpool_mutex;
static std::mutex stderr_mutex;
static std::mutex pathcache_mutex;
static std::condition_variable condition;
static std::queue<std::function<void()> > Queue;
static bool terminate_pool;

struct job
	{
	std::string input;
	std::string output;
    std::vector<const char *> inc;
	};

void error_func( void * filename, const char * msg )
{
std::unique_lock<std::mutex> lock(stderr_mutex);
fprintf( stderr, "%s:%s\n", filename, msg );
}

static int compile_unit( job * j )
{
    int err = 0;
    TCCState* s = tcc_new();
    if(!s){
        printf("Can’t create a TCC context\n");
        err = -__LINE__;
        goto fail;
    }

    tcc_set_error_func(s, (void*)j->input.c_str(), error_func);

    tcc_set_output_type(s, TCC_OUTPUT_OBJ);

    for( unsigned i = 0; i < j->inc.size(); ++i ) {
        if( tcc_add_include_path(s, j->inc[i]) <0 ) {
            err = -__LINE__;
            goto fail;
        }
    }

    if( tcc_add_file(s, j->input.c_str()) <0) {
        printf("error adding file\n");
        err = -__LINE__;
        goto fail;
    }

    if( tcc_output_file(s, j->output.c_str()) < 0 ) {
        printf("outputfile error\n");
        err = -__LINE__;
        goto fail;
    }

fail:
    tcc_delete(s);
    delete( j );
    return err;
}

static void Infinite_loop_function()
{
    while(true)
    {
        std::unique_lock<std::mutex> lock(Queue_Mutex);
        condition.wait(lock, []{return !Queue.empty() || terminate_pool;});
        if( terminate_pool && Queue.empty() ) return;
        std::function<void()> Job = Queue.front();
        Queue.pop();
        lock.unlock();
        Job(); // function<void()> type
    }
};

static void Add_Job(std::function<void()> New_Job)
{
    {
        std::unique_lock<std::mutex> lock(Queue_Mutex);
        Queue.push(New_Job);
    }
    condition.notify_one();
}

static int PendingJob()
{
int retn;
std::unique_lock<std::mutex> lock(Queue_Mutex);
return !Queue.empty();
}

int main(int argc, const char *argv[])
{
    int Num_Threads = std::thread::hardware_concurrency();
    std::vector<std::thread> Pool;
    fprintf(stderr, "Using %i threads\n",Num_Threads);
    for(int ii = 0; ii < Num_Threads; ii++)
    {  Pool.push_back(std::thread(Infinite_loop_function));}

    std::vector<const char *> includes;

    for( unsigned i = 1; i < argc; ++i )
    {
    job * j = new job;

    j->input = argv[i];

    //j->output = "out/";
    j->output.append(j->input);
    j->output.append(".1.o");

    //j->inc

    std::cout<<"queueing " << j->input << "->"<< j->output <<std::endl;
    #if MULTITHREAD
        Add_Job(std::bind(&compile_unit, j));
    #else
        compile_unit(j);
    #endif
    }

    std::unique_lock<std::mutex> lock(threadpool_mutex);
    terminate_pool = true; // use this flag in condition.wait

    condition.notify_all(); // wake up all threads.

    // Join all threads.
    for(std::thread &every_thread : Pool)
    {   every_thread.join();  }

    Pool.clear();

    return 0;
}

