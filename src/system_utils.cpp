/* 
 * Copyright 2017, OpenALPR Technology, Inc.  
 * All rights reserved
 * This file is part of the RollingDB library
 * RollingDB is licensed under LGPL
 */

#include "system_utils.h"

namespace rollingdbsupport
{

  void sleep_ms(int sleepMs)
  {
      #ifdef _WIN32
              Sleep(sleepMs);
      #else
              usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
      #endif
  }
  

  bool hasEnding (std::string const &fullString, std::string const &ending)
  {
    if (fullString.length() >= ending.length())
    {
      return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
      return false;
    }
  }





  bool hasEndingInsensitive(const std::string& fullString, const std::string& ending)
  {
    if (fullString.length() < ending.length())
      return false;

    int startidx = fullString.length() - ending.length();

    for (unsigned int i = startidx; i < fullString.length(); ++i)
        if (tolower(fullString[i]) != tolower(ending[i - startidx]))
            return false;
    return true;
  }

  bool DirectoryExists( const char* pzPath )
  {
    if ( pzPath == NULL) return false;

    DIR *pDir;
    bool bExists = false;

    pDir = opendir (pzPath);

    if (pDir != NULL)
    {
      bExists = true;
      (void) closedir (pDir);
    }

    return bExists;
  }

  bool fileExists( const char* pzPath )
  {
    if (pzPath == NULL) return false;

    bool fExists = false;
    std::ifstream f(pzPath);
    fExists = f.is_open();
    f.close();
    return fExists;
  }

  
  std::string get_directory_from_path(std::string file_path)
  {
    if (DirectoryExists(file_path.c_str()))
      return file_path;
    
    size_t found;
    
    found=file_path.find_last_of("/\\");
    
    if (found >= 0)
      return file_path.substr(0,found);
    
    return "";
  }
  
  std::vector<std::string> getFilesInDir(const char* dirPath)
  {
    DIR *dir;

    std::vector<std::string> files;

    struct dirent *ent;
    if ((dir = opendir (dirPath)) != NULL)
    {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL)
      {
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
          files.push_back(ent->d_name);
      }
      closedir (dir);
    }
    else
    {
      /* could not open directory */
      perror ("");
      return files;
    }

    return files;
  }
  
  std::string filenameWithoutExtension(std::string filename)
  {
    int lastslash = filename.find_last_of("/");
    if (lastslash >= filename.size())
      lastslash = 0;
    else
      lastslash += 1;
    
    int lastindex = filename.find_last_of(".");
    
    return filename.substr(lastslash, lastindex - lastslash);
  }
  
  
  std::vector<unsigned char> ReadAllBytes(char const* filename)
  {
      std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
      std::ifstream::pos_type pos = ifs.tellg();

      std::vector<unsigned char>  result(pos);

      ifs.seekg(0, std::ios::beg);
      ifs.read(reinterpret_cast<char*>(&result[0]), pos);

      return result;
  }

  static int makeDir(const char *path, mode_t mode)
  {
    struct stat            st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
      /* Directory does not exist. EEXIST for race condition */
      if (mkdir(path, mode) != 0 && errno != EEXIST)
        status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
      errno = ENOTDIR;
      status = -1;
    }

    return(status);
  }
  
  timespec diff(timespec start, timespec end);

  #ifdef _WIN32

  // Windows timing code
  LARGE_INTEGER getFILETIMEoffset()
  {
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
  }

  int clock_gettime(int X, timespec *tv)
  {
    LARGE_INTEGER           t;
    FILETIME            f;
    double                  microseconds;
    static LARGE_INTEGER    offset;
    static double           frequencyToMicroseconds;
    static int              initialized = 0;
    static BOOL             usePerformanceCounter = 0;

    if (!initialized)
    {
      LARGE_INTEGER performanceFrequency;
      initialized = 1;
      usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
      if (usePerformanceCounter)
      {
        QueryPerformanceCounter(&offset);
        frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
      }
      else
      {
        offset = getFILETIMEoffset();
        frequencyToMicroseconds = 10.;
      }
    }
    if (usePerformanceCounter) QueryPerformanceCounter(&t);
    else
    {
      GetSystemTimeAsFileTime(&f);
      t.QuadPart = f.dwHighDateTime;
      t.QuadPart <<= 32;
      t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = microseconds;
    tv->tv_sec = t.QuadPart / 1000000;
    tv->tv_usec = (t.QuadPart % 1000000)*1000;
    return (0);
  }

  void getTimeMonotonic(timespec* time)

  {
    clock_gettime(0, time);
  }
  
  int64_t getTimeMonotonicMs()
  {
    timespec time;
    getTimeMonotonic(&time);

    timespec time_start;
    time_start.tv_sec = 0;
    time_start.tv_usec = 0;

    return diffclock(time_start, time);
  }
  
  
  double diffclock(timespec time1,timespec time2)
  {
    timespec delta = diff(time1,time2);
    double milliseconds = (delta.tv_sec * 1000) +  (((double) delta.tv_usec) / 1000000.0);

    return milliseconds;
  }

  timespec diff(timespec start, timespec end)
  {
    timespec temp;
    if ((end.tv_usec-start.tv_usec)<0)
    {
      temp.tv_sec = end.tv_sec-start.tv_sec-1;
      temp.tv_usec = 1000000000+end.tv_usec-start.tv_usec;
    }
    else
    {
      temp.tv_sec = end.tv_sec-start.tv_sec;
      temp.tv_usec = end.tv_usec-start.tv_usec;
    }
    return temp;
  }


  int64_t getEpochTimeMs()
  {
    struct _timeb timebuffer;  
    _ftime64_s( &timebuffer );  
    return timebuffer.time * 1000 + timebuffer.millitm;
  } 

  #else

  void _getTime(bool realtime, timespec* time)
  {
    #if defined(__APPLE__) && defined(__MACH__) // OS X does not have clock_gettime, use clock_get_time
      clock_serv_t cclock;
      mach_timespec_t mts;
      host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
      clock_get_time(cclock, &mts);
      mach_port_deallocate(mach_task_self(), cclock);
      time->tv_sec = mts.tv_sec;
      time->tv_nsec = mts.tv_nsec;
    #else
      if (realtime)
        clock_gettime(CLOCK_REALTIME, time);
      else
        clock_gettime(CLOCK_MONOTONIC, time);
    #endif
  }
  
  // Returns a monotonic clock time unaffected by time changes (e.g., NTP)
  // Useful for interval comparisons
  void getTimeMonotonic(timespec* time)
  {
    _getTime(false, time);
  }
  
  int64_t getTimeMonotonicMs()
  {
    timespec time;
    getTimeMonotonic(&time);

    timespec time_start;
    time_start.tv_sec = 0;
    time_start.tv_nsec = 0;

    return diffclock(time_start, time);
  }

  double diffclock(timespec time1,timespec time2)
  {
    timespec delta = diff(time1,time2);
    double milliseconds = (((double) delta.tv_sec) * 1000.0) +  (((double) delta.tv_nsec) / 1000000.0);

    return milliseconds;
  }

  timespec diff(timespec start, timespec end)
  {
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0)
    {
      temp.tv_sec = end.tv_sec-start.tv_sec-1;
      temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    }
    else
    {
      temp.tv_sec = end.tv_sec-start.tv_sec;
      temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
  }


  // Returns wall clock time since Unix epoch (Jan 1, 1970)
  int64_t getEpochTimeMs()
  {
    timespec time;
    _getTime(true, &time);

    timespec epoch_start;
    epoch_start.tv_sec = 0;
    epoch_start.tv_nsec = 0;

    return diffclock(epoch_start, time);

  } 

  #endif

  
  
  #ifdef _WIN32
  // Stub out these functions on Windows.  They're used for the daemon anyway, which isn't supported on Windows.

  bool makePath(const char* path, mode_t mode) {
	  std::stringstream pathstream;
	  pathstream << "mkdir \"" << path << "\"";
	  std::string candidate_path = pathstream.str();
	  std::replace(candidate_path.begin(), candidate_path.end(), '/', '\\');

	  system(candidate_path.c_str());
	  return true; 
  }
  FileInfo getFileInfo(std::string filename) { 
    FileInfo response;
    response.creation_time = 0;
    response.size = 0;
    return response;
  }

  
  #else

  
  /**
  ** makePath - ensure all directories in path exist
  ** Algorithm takes the pessimistic view and works top-down to ensure
  ** each directory in path exists, rather than optimistically creating
  ** the last element and working backwards.
  */
  bool makePath(const char* path, mode_t mode)
  {

    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = makeDir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = makeDir(path, mode);
    free(copypath);
    return (status == 0);

  }
  
  
  #endif
}