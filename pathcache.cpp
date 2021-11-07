#include <map>
#include <set>
#include <string>
#include <cstring>

#include <filesystem>

#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>

#include <limits.h>

#include <mutex>
#include <condition_variable>
#include <shared_mutex>

#include <iostream>

typedef std::shared_mutex lock;
typedef std::unique_lock<lock> writeLock;
typedef std::shared_lock<lock> readLock;

enum dirState
  {
  DIRSTATE_LOADING,
  DIRSTATE_DONE
  };

struct dirCacheEntry
  {
  dirState s;
  std::mutex m;
  std::condition_variable cv;
  std::set<std::string> files;
  void wait()
    {
    if( s == DIRSTATE_DONE ) return;
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [this]{return s == DIRSTATE_DONE;});
    }
  };

static lock dirCacheLock;
static std::map<std::string, dirCacheEntry*> dirCache;

void dumpCache()
{
readLock rdlck( dirCacheLock );
for(const auto & entry : dirCache)
  {
  std::cout <<"dirCache:" << entry.first << std::endl;
  for(const auto & dirEntry : entry.second->files )
    {
    std::cout << "\t" << dirEntry << std::endl;
    }
  }
}

typedef int (*real_openat_t)(int, const char *, int, ...);

static int real_openat(int dirfd, const char *pathname, int flags, ...) {
  static real_openat_t cache = NULL;
  if( !cache ) cache = (real_openat_t)dlsym(RTLD_NEXT, "openat");
  return cache( dirfd, pathname, flags );
}

dirCacheEntry * dirCacheLookup( std::string dir )
{
//std::cout<<" dirCacheLookup("<<dir<<")"<<std::endl;
readLock lck( dirCacheLock );
std::map<std::string,dirCacheEntry*>::iterator it = dirCache.find( dir );
if (it != dirCache.end())
    return (it->second);
return NULL;
}

std::string cppbasename( std::string input )
{
char tmp[PATH_MAX];
strncpy( tmp, input.c_str(), sizeof(tmp) );
tmp[sizeof(tmp)-1] = 0;
return std::string( basename( tmp ) );
}

std::string cppdirname( std::string input )
{
char tmp[PATH_MAX];
strncpy( tmp, input.c_str(), sizeof(tmp) );
tmp[sizeof(tmp)-1] = 0;
return std::string( dirname( tmp ) );
}

dirCacheEntry * dirCacheBuildEntry( std::string dir )
{
writeLock wrlck( dirCacheLock );
dirCacheEntry * retn;

std::map<std::string,dirCacheEntry*>::iterator it = dirCache.find( dir );
if (it == dirCache.end())
  {
  //Need a new one
  retn = new dirCacheEntry;
  retn->s = DIRSTATE_LOADING;
  dirCache[dir] = retn;

  //Release pathcache lock to other threads while we build this dirCacheEntry
  wrlck.unlock();

  //Load files here
  try
  {
  for (const auto & entry : std::filesystem::directory_iterator(dir))
    {
    //std::string base_filename = cppbasename( entry.path().string() );
    if( std::filesystem::is_directory( entry ) )
      {
      //std::cout<<"Skipping directory " << entry.path().filename() << " in " << dir << std::endl;
      }
    else
      {
      //std::cout<<"Adding " << entry.path().filename() << " to " << dir << std::endl;
      retn->files.insert( entry.path().filename() );
      }
    }
  }
  catch (std::filesystem::filesystem_error )
  {
    //If the directory is missing, not much to do here
  }

  //std::cout<<"all files loaded from "<<dir<<", notifying"<<std::endl;
  //Signal files are loaded
  std::unique_lock<std::mutex> lk(retn->m);
  retn->s = DIRSTATE_DONE;
  retn->cv.notify_all();
  //std::cout<<"all files loaded from "<<dir<<", notify done"<<std::endl;
  }
else
  {
  //Done with the map
  retn = it->second;
  wrlck.unlock();

  //Wait for current one
  //std::cout<<"Waiting"<<std::endl;
  retn->wait();
  }
return retn;
}

extern "C" {

int openat(int dirfd, const char *pathname, int flags, ...)
{
if( dirfd != AT_FDCWD ) return real_openat( dirfd, pathname, flags );
if( flags ) return real_openat( dirfd, pathname, flags );
//return real_openat( dirfd, pathname, flags );

//std::cout<<"open(" << pathname <<"," <<flags <<")" << std::endl;

std::string dir = cppdirname( pathname );

dirCacheEntry * dirCacheEntryPtr = dirCacheLookup( dir );
if( dirCacheEntryPtr )
  {
  //std::cout<<"got cache entry for " << pathname <<std::endl;
  dirCacheEntryPtr->wait();
  }
else
  {
  //std::cout<<"building cache entry for "<< pathname << std::endl;
  dirCacheEntryPtr = dirCacheBuildEntry( dir );
  //std::cout<<"got built cache entry for " << pathname <<std::endl;
  }


std::string basestr = cppbasename( pathname );

if( dirCacheEntryPtr->files.find( basestr ) != dirCacheEntryPtr->files.end() )
  {
  //std::cout<<"dirCache had entry for "<<basestr<<std::endl;
  return real_openat( dirfd, pathname, flags );
  }

//std::cout<<"dirCache had no entry for "<<basestr<<std::endl;
return -1;
}

int open(const char *pathname, int flags, ...)
{
return openat( AT_FDCWD, pathname, flags );
}

}//extern c
