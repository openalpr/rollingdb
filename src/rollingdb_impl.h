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


#include <alprsupport/tinythread.h>
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
  RollingDBImpl(std::string chunk_directory, int max_size_gb, int jpeg_quality, log4cplus::Logger logger, bool read_only = false);
  virtual ~RollingDBImpl();
  
  void write_image(std::string name, cv::Mat image);
  void write_image(std::string name, std::vector<uchar>& image_bytes);
  
  bool read_image(std::string name, cv::Mat& output_image);
  bool read_image(std::string name, std::vector<uchar>& image_bytes);
  
  bool active;
  
  void reload_from_disk();
private:

  bool readonly;
  
  int jpeg_quality;
  
  log4cplus::Logger logger;
  SharedArchiveThreadData archive_thread_data;
  
  tthread::thread* thread_writeimage;
  tthread::thread* thread_watchdir;
  
};

#endif /* ROLLINGDB_IMPL_H */

