/* 
 * Copyright 2017, OpenALPR Technology, Inc.  
 * All rights reserved
 * This file is part of the RollingDB library
 * RollingDB is licensed under LGPL
 */

#include "rollingdb_impl.h"

#include <log4cplus/loggingmacros.h>
#include <iostream>
#include "system_utils.h"
#include <set>
#ifndef WIN32
#include <sys/inotify.h>
#endif
#include <mutex>


using namespace std;

std::mutex image_list_mutex;

void imageWriteThread(void* arg);
void directoryWatchThread(void* arg);
  
  
bool imgCompare( const LmdbEntry &leftImage, const LmdbEntry &rightImage );

RollingDBImpl::RollingDBImpl(std::string chunk_directory, int max_size_gb, log4cplus::Logger logger, bool read_only)
{
  this->logger = logger;
  this->readonly = read_only;
  
  const float MEGABYTES_IN_A_GB = 1024;
  int max_num_chunks = (((float) max_size_gb) * MEGABYTES_IN_A_GB) / MEGABYTES_PER_LMDB_CHUNK;
  
  LOG4CPLUS_INFO(logger, "Image Archive: Initialized archive.  Path: " << chunk_directory << " - Maximum of " << max_num_chunks << " chunks at " << MEGABYTES_PER_LMDB_CHUNK << " MB per chunk");
  
  archive_thread_data.logger = logger;
  archive_thread_data.chunk_manager = new ChunkCollection(chunk_directory, max_num_chunks, logger, read_only);
  archive_thread_data.active = true;

  if (!readonly)
    thread_writeimage = new std::thread(imageWriteThread, (void*) &archive_thread_data);
  
  thread_watchdir = new std::thread(directoryWatchThread, (void*) &archive_thread_data);
  
}


RollingDBImpl::~RollingDBImpl() {

  archive_thread_data.active = false;
  if (!readonly && thread_writeimage->joinable())
    thread_writeimage->join();
  
  thread_watchdir->join();
  
  delete archive_thread_data.chunk_manager;
  
}





bool RollingDBImpl::read_blob(std::string key, std::vector<unsigned char>& image_bytes) {

  timespec startTime, endTime;
  rollingdbsupport::getTimeMonotonic(&startTime);
  int64_t epoch_time = LmdbChunk::parse_lmdb_epoch_time(key);
  if (epoch_time < 0)
    return false;
      
  // Get the epoch time from the string.  Assume the last set of 30 characters before
  // the end is our epoch time
  LmdbChunk chunk;
  
  LOG4CPLUS_INFO(logger, "Image Archive: Searching for " << epoch_time);
  
  bool has_chunk = archive_thread_data.chunk_manager->get_chunk_path(epoch_time, chunk);
  
  if (!has_chunk)
  {
    LOG4CPLUS_INFO(logger, "Image Archive: requested time (" << epoch_time << ") is before beginning of image db");
    return false;
  }
  
  
  ReadStatus status = chunk.read_image(key, image_bytes);
  
  LOG4CPLUS_INFO(logger, "Image Archive: readstatus: " << status);
  if (status != READ_SUCCESS)
    return false;
  
  rollingdbsupport::getTimeMonotonic(&endTime);
  double readTime = rollingdbsupport::diffclock(startTime, endTime);
  
  LOG4CPLUS_DEBUG(logger, "Image Archive: Read took: " << readTime << " ms.");
  return true;
}


void RollingDBImpl::reload_from_disk() {
  archive_thread_data.chunk_manager->reload();
}

void RollingDBImpl::write_blob(std::string name, uint64_t epoch_time_ms, std::vector<unsigned char>& image_bytes) {
  const int MAX_IMAGES_IN_WRITE_BUFFER = 1000;
  
  if (readonly)
  {
    LOG4CPLUS_ERROR(logger, "Image Archive: Attempting to write to a read-only database");
    return;
  }
    
  const uint64_t THE_YEAR_2300 = 10413835200000;
  if (epoch_time_ms > THE_YEAR_2300)
  {
    LOG4CPLUS_DEBUG(logger, "Image Archive: Invalid epoch time: " << epoch_time_ms);
    return;
  }
  
  stringstream key;
  key << name << "-" << epoch_time_ms;
  LmdbEntry entry;
  entry.image_bytes = image_bytes;
  entry.epoch_time = epoch_time_ms;
  entry.key = key.str();
  
  
  // Add to the write queue.  Don't buffer an unlimited amount (we'd run out of memory if the disk is out))
  image_list_mutex.lock();
  if (archive_thread_data.image_write_list.size() < MAX_IMAGES_IN_WRITE_BUFFER)
    archive_thread_data.image_write_list.push(entry);
  else
    LOG4CPLUS_WARN(logger, "Image Archive: Image buffer write overflow. Dropping images from write queue.");
  image_list_mutex.unlock();
}

int RollingDBImpl::get_write_buffer_size() {
  return archive_thread_data.image_write_list.size();
}

void imageWriteThread(void* arg)
{
  const int MIN_MILLISECONDS_TO_TRIGGER = 2500;
  const int MIN_IMAGES_TO_TRIGGER = 10;
  const int MAX_IMAGES_PER_WRITE = 20;

  SharedArchiveThreadData* tdata = (SharedArchiveThreadData*) arg;
 
  LOG4CPLUS_DEBUG(tdata->logger, "Starting write thread: " << tdata->active);
  
  uint64_t last_push = rollingdbsupport::getEpochTimeMs();
  while (tdata->active)
  {
    uint64_t now = rollingdbsupport::getEpochTimeMs();

    if ((now - last_push > MIN_MILLISECONDS_TO_TRIGGER || tdata->image_write_list.size() > MIN_IMAGES_TO_TRIGGER) &&
        (tdata->image_write_list.size() > 0))
    {
      
      vector<LmdbEntry> entriesCopy;
      
      image_list_mutex.lock();
      int starting_list_size = tdata->image_write_list.size();
      for (size_t i = 0; i < starting_list_size; i++)
      {
        if (i >= MAX_IMAGES_PER_WRITE || !tdata->active)
          break;
        
        entriesCopy.push_back(tdata->image_write_list.top());
        tdata->image_write_list.pop();
      }

      WriteStatus status = UNKNOWN_WRITE_FAILURE;
      
      // Keep looping and attempting to rewrite if there are failures
      while (status != WRITE_SUCCESS)
      {

        LmdbChunk chunk;
        bool has_active = tdata->chunk_manager->get_active_chunk(chunk);
        if (!has_active)
        {
          chunk = tdata->chunk_manager->new_chunk(entriesCopy[0].epoch_time);
        }
        image_list_mutex.unlock();

        status = chunk.write_image(entriesCopy);

        if (status == DATABASE_FULL)
        {
          image_list_mutex.lock();
          LmdbChunk newChunk = tdata->chunk_manager->new_chunk(entriesCopy[0].epoch_time);
          status = newChunk.write_image(entriesCopy);
          image_list_mutex.unlock();
          break;
        }
        else if (status != WRITE_SUCCESS)
        {
          LOG4CPLUS_WARN(tdata->logger, "Image Archive: Failed writing image to database: " << status);
        }
        else
        {
          break;
        }
        
        LOG4CPLUS_WARN(tdata->logger, "Image Archive: Write failed, retrying");
        rollingdbsupport::sleep_ms(500);
      }

    }
    
    rollingdbsupport::sleep_ms(100);
  }
  
  LOG4CPLUS_INFO(tdata->logger, "Image Archive: Exiting write thread: " << tdata->active);
  
}



// Watches directory for new LMDB files.  If it finds any, it updates the collection
void directoryWatchThread(void* arg)
 {
  SharedArchiveThreadData* tdata = (SharedArchiveThreadData*) arg;
  
  #ifdef _WIN32
  std::set<std::string> known_files;
  // On Cygwin/windows
  while (tdata->active)
  {
    // Every second poll for directory changes
    vector<string> files = rollingdbsupport::getFilesInDir(tdata->chunk_manager->chunk_directory.c_str());
    std::set<std::string> latest_files;
    
    for (uint32_t i = 0; i < files.size(); i++)
    {
      if (rollingdbsupport::hasEnding(files[i], ".mdb"))
      {
        latest_files.insert(files[i]);
      }
    }
    
    // Check for new files that we didn't know about
    for (std::set<string>::iterator it=latest_files.begin(); it!=latest_files.end(); ++it)
    {
      if (known_files.find(*it) == known_files.end())
      {
        // New file, add it to the list
        LOG4CPLUS_INFO(tdata->logger, "Image archive db file " << *it << " was created." );      
        int64_t epoch_time = LmdbChunk::parse_database_epoch_time(*it);
        tdata->chunk_manager->push_chunk(epoch_time);
        known_files.insert(*it);
      }
    }

    
    // Check for files that were removed
    std::vector<string> dellist;
    for (std::set<string>::iterator it=known_files.begin(); it!=known_files.end(); ++it)
    {
      if (latest_files.find(*it) == latest_files.end())
      {
        // file is deleted, remove it from the list
        LOG4CPLUS_INFO(tdata->logger, "Image archive db file " << *it << " was deleted." );  
        int64_t epoch_time = LmdbChunk::parse_database_epoch_time(*it);
        tdata->chunk_manager->pop_chunk(epoch_time);
       dellist.push_back(*it);
      }
    }
    for (uint32_t i = 0; i < dellist.size(); i++)
      known_files.erase(dellist[i]);
 
    rollingdbsupport::sleep_ms(1000);
  }
  #else
  
  const int MIN_MILLISECONDS_BETWEEN_FULLSCAN = 1 * 60 * 60 * 1000; // 1 hour * 60 minutes, seconds, milliseconds
    
  const int EVENT_SIZE =  ( sizeof (struct inotify_event) );
  const int BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) );
  
  int fd;
  int wd;
  fd_set descriptors; 
  struct timeval time_to_wait;
  int return_value = 0;


  char buffer[BUF_LEN];
  
  
  fd = inotify_init();

  if ( fd < 0 ) {
    LOG4CPLUS_ERROR(tdata->logger, "Image archive error initializing directory watch.");
    return;
  }

  LOG4CPLUS_INFO(tdata->logger, "Image archive watching: " << tdata->chunk_manager->chunk_directory );
  
  wd = inotify_add_watch( fd, tdata->chunk_manager->chunk_directory.c_str(), IN_DELETE | IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM  );
  
  if (wd < 0)
  {
    LOG4CPLUS_ERROR(tdata->logger, "Image archive error creating directory watch.");
  }

  FD_ZERO ( &descriptors );

  int64_t last_fullscan_timestamp = rollingdbsupport::getEpochTimeMs();
    
  // Keep looping waiting for the directory to change
  while (tdata->active)
  {
    
      
    if (rollingdbsupport::getEpochTimeMs() - last_fullscan_timestamp > MIN_MILLISECONDS_BETWEEN_FULLSCAN)
    {
      // Do a full scan on the database
      vector<string> filelist = rollingdbsupport::getFilesInDir(tdata->chunk_manager->chunk_directory.c_str());
      for (uint32_t i = 0; i < filelist.size(); i++)
      {
        if (rollingdbsupport::hasEndingInsensitive(filelist[i], IMAGE_DB_POSTFIX) == true)
        {
          int64_t epoch_time = LmdbChunk::parse_database_epoch_time(filelist[i]);
          tdata->chunk_manager->push_chunk(epoch_time);
        }
      }
      last_fullscan_timestamp = rollingdbsupport::getEpochTimeMs();
    }
    
    time_to_wait.tv_sec = 0;
    time_to_wait.tv_usec = 1000 * 750; // Wait 750 milliseconds between polls
    FD_SET ( fd, &descriptors );
    return_value = select ( fd + 1, &descriptors, NULL, NULL, &time_to_wait);
    
    if ( return_value < 0 ) {
	LOG4CPLUS_ERROR(tdata->logger, "Image archive error starting directory watcher.");
    }
    else if ( ! return_value ) {
      // No issue, just no changes.  Keep looping
    }
    else if ( FD_ISSET ( fd, &descriptors ) ) {
      /* Process the inotify events */
      int length = read( fd, buffer, BUF_LEN ); 

      if ( length < 0 ) {
        LOG4CPLUS_ERROR(tdata->logger, "Image archive error reading inotify directory.");

      }
      
      // Rest for a moment to allow other ops to take place (e.g., create a file, delete an old file)
      // This way we only do one refresh, rather than multiple.
      rollingdbsupport::sleep_ms(100);
      
      int i = 0;
      image_list_mutex.lock();
      while ( i < length ) {
        
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
        if ( event->len ) {
          if (rollingdbsupport::hasEndingInsensitive(string(event->name), IMAGE_DB_POSTFIX))
          {
            int64_t epoch_time = LmdbChunk::parse_database_epoch_time(event->name);
            
            if (epoch_time >= 0 && !(event->mask & IN_ISDIR))
            {
              
                      
              if ( event->mask & (IN_CREATE | IN_MOVED_TO) ) {
                tdata->chunk_manager->push_chunk(epoch_time);
                LOG4CPLUS_INFO(tdata->logger, "Image archive db file " << event->name << " was created." );       
              }
              else if ( event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                tdata->chunk_manager->pop_chunk(epoch_time);
                LOG4CPLUS_INFO(tdata->logger, "Image archive db file " << event->name << " was deleted." );       
              }
            }
          }
        }
        
        i += EVENT_SIZE + event->len;
      }
      image_list_mutex.unlock();
    }
    
  }

  ( void ) inotify_rm_watch( fd, wd );
  ( void ) close( fd );

  #endif

  LOG4CPLUS_INFO(tdata->logger, "Image archive directory watcher exiting.");
}
