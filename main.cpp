#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <libtcc.h>

static std::mutex Queue_Mutex;
static std::mutex threadpool_mutex;
static std::mutex stderr_mutex;
static std::condition_variable condition;
static std::queue<std::function<void()> > Queue;
static bool terminate_pool;

void error_func( void * opaque, const char * msg )
{
std::unique_lock<std::mutex> lock(stderr_mutex);
fprintf( stderr, "%s\n", msg );
}

static int compile_unit( const char * input, const char * output, std::vector<const char*> & inc )
{
    TCCState* s = tcc_new();
    if(!s){
        printf("Can’t create a TCC context\n");
        return 1;
    }

    tcc_set_error_func(s, NULL, error_func);

    tcc_set_output_type(s, TCC_OUTPUT_OBJ);

    if( tcc_add_file(s, input) <0) {
        printf("error adding file\n");
        return 2;
    }

    if( tcc_output_file(s, output) < 0 ) {
        printf("outputfile error\n");
        return 3;
    }

    tcc_delete(s);
    return 0;
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

int main(int argc, char **argv)
{
    int Num_Threads = std::thread::hardware_concurrency();
    std::vector<std::thread> Pool;
    fprintf(stderr, "Using %i threads\n",Num_Threads);
    for(int ii = 0; ii < Num_Threads; ii++)
    {  Pool.push_back(std::thread(Infinite_loop_function));}

    std::vector<const char *> includes;

#if 1
    Add_Job(std::bind(&compile_unit, "test/main.c", "test/main.o", includes));
    Add_Job(std::bind(&compile_unit, "test/funcA.c", "test/funcA.o", includes));
    Add_Job(std::bind(&compile_unit, "test/funcB.c", "test/funcB.o", includes));
    Add_Job(std::bind(&compile_unit, "test/funcC.c", "test/funcC.o", includes));
#else
    compile_unit("test/main.c","test/main.o", includes);
    compile_unit("test/funcA.c","test/funcA.o", includes);
    compile_unit("test/funcB.c","test/funcB.o", includes);
    compile_unit("test/funcC.c","test/funcC.o", includes);
#endif

    std::unique_lock<std::mutex> lock(threadpool_mutex);
    terminate_pool = true; // use this flag in condition.wait

    condition.notify_all(); // wake up all threads.

    // Join all threads.
    for(std::thread &every_thread : Pool)
    {   every_thread.join();  }

    Pool.clear();

    return 0;
}

