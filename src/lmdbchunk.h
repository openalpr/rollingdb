/* 
 * Copyright 2017, OpenALPR Technology, Inc.  
 * All rights reserved
 * This file is part of the RollingDB library
 * RollingDB is licensed under LGPL
 */

#ifndef OPENALPR_LMDBCHUNK_H
#define	OPENALPR_LMDBCHUNK_H

#include "lmdb.h"

#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>

#include <system_utils.h>

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

enum WriteStatus
{
  WRITE_SUCCESS,
  DATABASE_FULL,
  DATABASE_NOT_ACTIVE,
  UNKNOWN_WRITE_FAILURE
};

enum ReadStatus
{
  READ_SUCCESS,
  IMAGE_NOT_FOUND,
  UNKNOWN_READ_FAILURE
};

struct LmdbEntry
{
  bool operator()( const LmdbEntry &leftImage, const LmdbEntry &rightImage )
  {
    return leftImage.epoch_time > rightImage.epoch_time;

  }
  
  std::string key;
  int64_t epoch_time;
  std::vector<unsigned char> image_bytes;
};

// Use a smaller chunk size for 32-bit
#if INTPTR_MAX == INT32_MAX 
    #define MEGABYTES_PER_LMDB_CHUNK 250
#elif _WIN32
    #define MEGABYTES_PER_LMDB_CHUNK 500
#elif INTPTR_MAX == INT64_MAX
    #define MEGABYTES_PER_LMDB_CHUNK 1000
#else
    #error "Environment not 32 or 64-bit."
    #define MEGABYTES_PER_LMDB_CHUNK 500
#endif

class LmdbChunk {
public:
  LmdbChunk();
  LmdbChunk(std::string db_path);
  virtual ~LmdbChunk();
  
  void setDbPath(std::string db_path);
  
  // Opens the environment.  Returns true if successful
  bool setActive(bool active);
  bool isActive();
  
  WriteStatus write_image(std::vector<LmdbEntry> images);
  ReadStatus read_image(std::string name, std::vector<unsigned char>& image_bytes);
  
  std::string db_fullpath;
  std::string lockfile_fullpath;
  int64_t epoch_time_start;
  
  void setLogger(log4cplus::Logger* logger);
  
  static int64_t parse_lmdb_epoch_time(std::string uuid);
  static int64_t parse_database_epoch_time(std::string database_file_name);
private:

  
  log4cplus::Logger* logger;
  MDB_env *env;
  
  bool open_env();
  void close_env();
  
};

#endif	/* OPENALPR_LMDBCHUNK_H */

