/* 
 * Copyright 2017, OpenALPR Technology, Inc.  
 * All rights reserved
 * This file is part of the RollingDB library
 * RollingDB is licensed under LGPL
 */

#ifndef ROLLINGDB_SYSTEM_UTILS_H
#define ROLLINGDB_SYSTEM_UTILS_H

#ifdef _WIN32
    // Import windows only stuff
	#include <windows.h>
	#include <sys/timeb.h>  

	#define timespec timeval

#else
    #include <sys/time.h>
#endif

// Support for OS X
#if defined(__APPLE__) && defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#if defined _WIN32
#include <dirent.h>

#else
#include <dirent.h>
#include <unistd.h>
#endif

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <errno.h>

namespace rollingdbsupport
{
  std::string filenameWithoutExtension(std::string filename);

  
  double diffclock(timespec time1,timespec time2);
  void getTimeMonotonic(timespec* time);
  int64_t getEpochTimeMs();
  
  void sleep_ms(int sleepMs);

  bool hasEnding (std::string const &fullString, std::string const &ending);
  bool hasEndingInsensitive(const std::string& fullString, const std::string& ending);
  bool DirectoryExists( const char* pzPath );
  bool fileExists( const char* pzPath );
  std::string get_directory_from_path(std::string file_path);

  std::string filenameWithoutExtension(std::string filename);
  std::vector<unsigned char> ReadAllBytes(char const* filename);
  
  bool makePath(const char* path, mode_t mode);
  std::vector<std::string> getFilesInDir(const char* dirPath);
}
#endif /* ROLLINGDB_SYSTEM_UTILS_H */

