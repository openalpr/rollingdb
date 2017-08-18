/* 
 * Copyright 2017, OpenALPR Technology, Inc.  
 * All rights reserved
 * This file is part of the RollingDB library
 * RollingDB is licensed under LGPL
 */

#ifndef OPENALPR_ROLLINGDB_H
#define	OPENALPR_ROLLINGDB_H


#include <string>

#include <log4cplus/logger.h>


class RollingDBImpl;
class RollingDB {
public:
  RollingDB(std::string chunk_directory, int max_size_gb, log4cplus::Logger logger, bool read_only = false);
  virtual ~RollingDB();
  
  void write_blob(std::string name, uint64_t epoch_time_ms, std::vector<unsigned char>& image_bytes);
  
  bool read_blob(std::string key, std::vector<unsigned char>& image_bytes);
  
  int get_write_buffer_size();
  
  bool active;
  
  void reload_from_disk();
  
private:
  RollingDBImpl* impl;
  
};

#endif	/* OPENALPR_ROLLINGDB_H */

