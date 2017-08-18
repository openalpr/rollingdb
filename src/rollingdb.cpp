/* 
 * Copyright 2017, OpenALPR Technology, Inc.  
 * All rights reserved
 * This file is part of the RollingDB library
 * RollingDB is licensed under LGPL
 */

#include "rollingdb.h"

#include "rollingdb_impl.h"


using namespace std;


RollingDB::RollingDB(std::string chunk_directory, int max_size_gb, log4cplus::Logger logger, bool read_only)
{
  this->impl = new RollingDBImpl(chunk_directory, max_size_gb, logger, read_only);
}


RollingDB::~RollingDB() {

  delete impl;
  
}




bool RollingDB::read_blob(std::string key, std::vector<unsigned char>& image_bytes) {
  return impl->read_blob(key, image_bytes);
}

void RollingDB::reload_from_disk() {
  impl->reload_from_disk();
}

void RollingDB::write_blob(std::string name, uint64_t epoch_time_ms, std::vector<unsigned char>& image_bytes) {
  impl->write_blob(name, epoch_time_ms, image_bytes);
}

int RollingDB::get_write_buffer_size() {
  return impl->get_write_buffer_size();
}

