#include <map>
#include <mutex>
#include <string>

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>

static std::mutex mutex;
static std::map<std::string, std::string> positive;
static std::map<std::string, int> negative;

extern "C" {

typedef int (*real_openat_t)(int, const char *, int, ...);

static int real_openat(int dirfd, const char *pathname, int flags, ...) {
  static real_openat_t cache = NULL;
  if( !cache ) cache = (real_openat_t)dlsym(RTLD_NEXT, "openat");
  return cache( dirfd, pathname, flags );
}

int openat(int dirfd, const char *pathname, int flags, ...)
{
if( dirfd != AT_FDCWD ) return real_openat( dirfd, pathname, flags );

std::unique_lock<std::mutex> lock(mutex);

std::string pathstr = pathname;

std::string base_filename = pathstr.substr(pathstr.find_last_of("/\\") + 1);

auto possearch = positive.find( base_filename );
if( possearch != positive.end() )
  {
//  printf("positive hit:%s->%s->%s\n", pathname, base_filename.c_str(), possearch->second.c_str() );
  return real_openat( dirfd, possearch->second.c_str(), flags );
  }

auto negsearch = negative.find( pathstr );
if( negsearch != negative.end() )
  {
//  printf("negative hit:%s\n", pathname );
  errno = negsearch->second;
  return -1;
  }

int retn = real_openat( dirfd, pathname, flags );
if( retn < 0 )
  {
//  printf("raw miss:%s\n", pathname);
  negative.insert( std::pair<std::string,int>(pathname,errno) );
  }
else
  {
//  printf("raw hit:%s\n", pathname);
  positive.insert( std::pair<std::string,std::string>(base_filename, pathname) );
  }

return retn;
}

int open(const char *pathname, int flags, ...)
{
return openat( AT_FDCWD, pathname, flags );
}

}//extern c
