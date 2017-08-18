/* 
 * File:   chunkcollection.cpp
 * Author: mhill
 * 
 * Created on April 1, 2016, 11:10 AM
 */

#include "chunkcollection.h"
#include "system_utils.h"
#include <sstream>
#include <algorithm>

using namespace std;


bool compareChunkDb(const LmdbChunk& firstElem, const LmdbChunk& secondElem) {
  return firstElem.epoch_time_start < secondElem.epoch_time_start;
}

  
ChunkCollection::ChunkCollection(std::string chunk_directory, int max_num_chunks, log4cplus::Logger logger, bool read_only) {
  
  this->logger = logger;
  this->readonly = read_only;
  // Clip off the trailing "/" if it exists
  if (rollingdbsupport::hasEnding(chunk_directory, "/") || rollingdbsupport::hasEnding(chunk_directory, "\\"))
    chunk_directory = chunk_directory.substr(0, chunk_directory.size() - 1);
  
  std::stringstream ss;
  ss << chunk_directory << "/" << IMAGE_DB_SUBDIR;
  
  this->chunk_directory=  ss.str();
  this->max_num_chunks = max_num_chunks;
  
  if (this->max_num_chunks <= 0)
    max_num_chunks = 1;
  
  reload();
}



ChunkCollection::~ChunkCollection() {
}

bool ChunkCollection::delete_database(LmdbChunk chunk_db) {
  
  // Don't delete a database if this is read-only
  if (readonly)
    return true;
  
  //cout << "Deleting: " << chunk_db.db_fullpath << endl;
  LOG4CPLUS_INFO(logger, "Image Archive: Deleting: " << chunk_db.db_fullpath);
  
  if (rollingdbsupport::fileExists(chunk_db.db_fullpath.c_str()))
  {
    int success_code = -1;
    int retry_count = 0;
    while (success_code != 0 && retry_count < 3)
    {
      success_code = remove(chunk_db.db_fullpath.c_str());
      if (success_code == 0)
        break;
      
      retry_count++;
      LOG4CPLUS_WARN(logger, "Image Archive: Failed to delete : " << chunk_db.db_fullpath << "... attempt #" << retry_count);
      rollingdbsupport::sleep_ms(1000);
    }
  }
  if (rollingdbsupport::fileExists(chunk_db.lockfile_fullpath.c_str()))
    remove(chunk_db.lockfile_fullpath.c_str());

  // Return true if file is gone
  return !rollingdbsupport::fileExists(chunk_db.db_fullpath.c_str());
}


void ChunkCollection::reload() {

  if (!rollingdbsupport::DirectoryExists(chunk_directory.c_str()))
  {
    rollingdbsupport::makePath(chunk_directory.c_str(), 0755);
  }
  
  std::list<LmdbChunk>::iterator it = chunk_db_files.begin();
  while (it != chunk_db_files.end())
  {
    if (it->isActive())
      it->setActive(false);
    it++;
  }
  
  chunk_db_files.clear();
  
  vector<string> all_files = rollingdbsupport::getFilesInDir(chunk_directory.c_str());
  
  for (size_t i = 0; i < all_files.size(); i++)
  {
    if (rollingdbsupport::hasEndingInsensitive(all_files[i], IMAGE_DB_POSTFIX))
    {
      stringstream ss;
      ss << chunk_directory << "/" << all_files[i];
      
      LmdbChunk a_chunk = LmdbChunk(ss.str());
      a_chunk.setLogger(&logger);
      
      chunk_db_files.push_back(a_chunk);
      
    }
  }
  
  chunk_db_files.sort(compareChunkDb);
  
//  for (int z = 0; z < chunk_db_files.size(); z++)
//    cout << chunk_db_files[z].db_fullpath << endl;

  
  // If we're already over the limit (e.g., someone updated the size settings and restarted)
  // start deleting databases until we're back under.
  
  // On reload, the last element is (likely) not completely full.  So it's unnecessary 
  // To delete an extra database.  Incremenet max_num_chunks prior to calling delete
  max_num_chunks++;
  delete_oldest_chunks();
  max_num_chunks--;
  
  if (chunk_db_files.size() > 0 && !readonly)
    chunk_db_files.back().setActive(true);
}

void ChunkCollection::delete_oldest_chunks() {
  
  if (readonly)
  {
    LOG4CPLUS_INFO(logger, "Image Archive: Not deleting chunks in Read Only thread ");
  }
  else
  {
    LOG4CPLUS_INFO(logger, "Image Archive: Deleting oldest chunks.  Current archive size (before delete) is " << chunk_db_files.size() << " / " << max_num_chunks << " chunks");
    
    int chunks_over_limit = chunk_db_files.size() - (max_num_chunks - 1);

    if (chunks_over_limit > 0)
    LOG4CPLUS_INFO(logger, "Image Archive: Deleting " << chunks_over_limit << " chunks.");
    
    for (int i = 0; i < chunks_over_limit; i++)
    {
      if (chunk_db_files.size() == 0)
        break;

      bool success = delete_database(*chunk_db_files.begin());
      if (success)
        chunk_db_files.erase(chunk_db_files.begin());
      else
      {
        LOG4CPLUS_WARN(logger, "Image Archive: Failed to delete image archive.  Aborting deletes");
        break;
      }
        
    }
  
  }
}

bool ChunkCollection::get_chunk_path(uint64_t epoch_time, LmdbChunk& chunk) {
  // Search through the available databases to find the most recent one
  // that has a name that is less than epoch_time
  // A binary tree would do this fast, but this probably isn't a bottleneck
   

  for (std::list<LmdbChunk>::reverse_iterator rit=chunk_db_files.rbegin(); rit!=chunk_db_files.rend(); ++rit)
  {
    if (epoch_time > rit->epoch_time_start)
    {
      chunk = *rit;
      
      return true;
    }
  }
  
  return false;
  
}

size_t ChunkCollection::size() {
  return chunk_db_files.size();
}

bool ChunkCollection::get_active_chunk(LmdbChunk& chunk)
{

  LmdbChunk* cur_active_chunk = NULL;
  // Check that there aren't two active chunks
  std::list<LmdbChunk>::iterator it = chunk_db_files.begin();
  while (it != chunk_db_files.end())
  {
    if (it->isActive())
    {
      if (cur_active_chunk != NULL)
      {
        LOG4CPLUS_WARN(logger, "Image Archive: Error, multiple chunks are active");
      }
      cur_active_chunk = &(*it);
    }
    it++;
  }
  
  
  if (cur_active_chunk == NULL)
    return false;
  
  chunk = *cur_active_chunk;
  return true;
    
    
}


LmdbChunk ChunkCollection::new_chunk(uint64_t epoch_time) {
  
  delete_oldest_chunks();
  
  // Check to make sure that it's not already in a database
  if (chunk_db_files.size() > 0)
  {
    if (chunk_db_files.back().epoch_time_start > epoch_time)
    {
      // We have a problem...  The last db start time is more recent than this new 
      // database will be...
      LOG4CPLUS_WARN(logger, "Image Archive: New database is older than last database. Bad stuff here... ");
    }
  }
  
  std::list<LmdbChunk>::iterator it = chunk_db_files.begin();
  while (it != chunk_db_files.end())
  {
    if (it->isActive())
      it->setActive(false);
    it++;
  }


  stringstream ss;
  ss << chunk_directory << "/" << epoch_time << IMAGE_DB_POSTFIX;

  LOG4CPLUS_INFO(logger, "Image Archive: Creating new image archive: " << ss.str());

  LmdbChunk lc = LmdbChunk(ss.str());
  lc.setLogger(&logger);

  if (!readonly)
    lc.setActive(true);

  chunk_db_files.push_back(lc);

  return lc;
  
}

void ChunkCollection::pop_chunk(uint64_t epoch_time) {

  std::list<LmdbChunk>::iterator it = chunk_db_files.begin();
  while (it != chunk_db_files.end())
  {
    if (it->epoch_time_start == epoch_time)
    {
      if (it->isActive())
        it->setActive(false);
      LOG4CPLUS_INFO(logger, "Image Archive: Removing reference to database " << epoch_time);
      chunk_db_files.erase(it);
      break;
    }
    it++;
  }
  

}

void ChunkCollection::push_chunk(uint64_t epoch_time) {

  std::list<LmdbChunk>::iterator it;
  
  LOG4CPLUS_INFO(logger, "Image Archive: Pushing db file to db " << epoch_time);
          
  stringstream ss;
  ss << chunk_directory << "/" << epoch_time << IMAGE_DB_POSTFIX;
  
  if (!rollingdbsupport::fileExists(ss.str().c_str()))
  {
      LOG4CPLUS_WARN(logger, "Image Archive: Attempting to add non-existent database. ");
      return;
  }
  
  // Check if the database for this epoch time already exists... if so, don't add it.
  it = chunk_db_files.begin();
  while (it != chunk_db_files.end())
  {
    if (it->epoch_time_start == epoch_time)
    {
      LOG4CPLUS_INFO(logger, "Image Archive: Database already referenced.  Skipping. " << epoch_time);
      return;
    }
    it++;
  }
  
  
  LmdbChunk lc = LmdbChunk(ss.str());
  lc.setLogger(&logger);
  
  // Iterate backwards through the list.  Once we find an element that has a smaller
  // epoch time than ours, insert our stuff after it.  If we make it to the beginning, insert at the beginning
  bool inserted = false;

  for (std::list<LmdbChunk>::reverse_iterator rit=chunk_db_files.rbegin(); rit!=chunk_db_files.rend(); ++rit)
  {
    if (rit->epoch_time_start < epoch_time)
    {
      chunk_db_files.insert(rit.base(), lc);
      
      inserted = true;
      break;
    }
  }
  if (!inserted)
    chunk_db_files.push_front(lc);
  
  
  it = chunk_db_files.begin();
  while (it != chunk_db_files.end())
  {
    it->setActive(false);
    it++;
  }

  if (chunk_db_files.size() > 0 && !readonly)
    chunk_db_files.back().setActive(true);
}
