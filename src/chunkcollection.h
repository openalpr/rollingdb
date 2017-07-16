/* 
 * File:   chunkcollection.h
 * Author: mhill
 *
 * Created on April 1, 2016, 11:10 AM
 */

#ifndef OPENALPR_CHUNKCOLLECTION_H
#define	OPENALPR_CHUNKCOLLECTION_H

#include <string>
#include <vector>
#include <stdint.h>
#include <list>
#include "lmdbchunk.h"
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#define IMAGE_DB_POSTFIX ".mdb"
#define IMAGE_DB_SUBDIR "image_db"

class ChunkCollection {
public:
  ChunkCollection(std::string chunk_directory, int max_num_chunks, log4cplus::Logger logger, bool read_only);
  virtual ~ChunkCollection();
  
  std::string chunk_directory;
  
  size_t size();
  
  bool get_chunk_path(uint64_t epoch_time, LmdbChunk& chunk);
  
  bool get_active_chunk(LmdbChunk& chunk);
  
  // Deletes the oldest chunk (if we're at the limit) and responds with the 
  // new chunk name.
  LmdbChunk new_chunk(uint64_t epoch_time);

  void pop_chunk(uint64_t epoch_time);
  void push_chunk(uint64_t epoch_time);
  
  void reload();
private:

  bool readonly;
  
  log4cplus::Logger logger;
  int max_num_chunks;
  
  void delete_oldest_chunks();
  
  bool delete_database(LmdbChunk chunk_db);
  
  std::list<LmdbChunk> chunk_db_files;
  
};

#endif	/* OPENALPR_CHUNKCOLLECTION_H */

