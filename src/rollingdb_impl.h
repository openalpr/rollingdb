/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   rollingdb_impl.h
 * Author: mhill
 *
 * Created on July 22, 2017, 2:44 PM
 */

#ifndef ROLLINGDB_IMPL_H
#define ROLLINGDB_IMPL_H

#include <string>
#include <queue>
#include <thread>


#include "lmdb.h"
#include "chunkcollection.h"


struct SharedArchiveThreadData
{
  std::priority_queue<LmdbEntry, std::vector<LmdbEntry>, LmdbEntry > image_write_list;
  ChunkCollection* chunk_manager;
  bool active;
  log4cplus::Logger logger;
};

class RollingDBImpl {
public:
  RollingDBImpl(std::string chunk_directory, int max_size_gb, log4cplus::Logger logger, bool read_only = false);
  virtual ~RollingDBImpl();
  
  void write_blob(std::string name, uint64_t epoch_time_ms, std::vector<unsigned char>& image_bytes);
  
  bool read_blob(std::string key, std::vector<unsigned char>& image_bytes);
  
  int get_write_buffer_size();
  
  bool active;
  
  void reload_from_disk();
private:

  bool readonly;
  
  
  log4cplus::Logger logger;
  SharedArchiveThreadData archive_thread_data;
  
  std::thread* thread_writeimage;
  std::thread* thread_watchdir;
  
};

#endif /* ROLLINGDB_IMPL_H */

