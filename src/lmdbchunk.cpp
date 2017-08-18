/* 
 * File:   lmdbchunk.cpp
 * Author: mhill
 * 
 * Created on April 1, 2016, 9:00 AM
 */

#include "lmdbchunk.h"
#include "system_utils.h"
#include <re2/re2.h>

using namespace std;
using namespace rollingdbsupport;


bool imgCompare( const LmdbEntry &leftImage, const LmdbEntry &rightImage )
{
  std::string left = leftImage.key;
  std::string right = rightImage.key;
  for( std::string::const_iterator lit = left.begin(), rit = right.begin(); lit != left.end() && rit != right.end(); ++lit, ++rit )
    if( tolower( *lit ) < tolower( *rit ) )
      return true;
    else if( tolower( *lit ) > tolower( *rit ) )
      return false;
  if( left.size() < right.size() )
    return true;
  return false;
}
  

LmdbChunk::LmdbChunk() {
  env = NULL;
  logger = NULL;
  
  setDbPath("");
}

LmdbChunk::LmdbChunk(std::string db_path) {
  env = NULL;
  logger = NULL;
  setDbPath(db_path);
}

void LmdbChunk::setLogger(log4cplus::Logger* logger) {
  this->logger = logger;
}


void LmdbChunk::setDbPath(std::string db_path) {

  this->db_fullpath = db_path;
  
  if (db_path == "")
  {
    this->lockfile_fullpath = "";
    this->epoch_time_start = -1;
  }
  else
  {
    stringstream sslock;
    sslock << rollingdbsupport::get_directory_from_path(db_fullpath) << "/" << 
            rollingdbsupport::filenameWithoutExtension(db_fullpath) << ".mdb-lock";

    this->lockfile_fullpath = sslock.str();

    epoch_time_start = parse_database_epoch_time(db_path);

    //cout << "Parsed " << db_path << " as " << epoch_time_start << endl;
  }
  
}


LmdbChunk::~LmdbChunk() {
  
}

bool LmdbChunk::setActive(bool active) {
  if (active && env == NULL)
  {
    return open_env();
  }
  else if (active && env != NULL)
  {
    return true;
  }
  else if (!active)
  {
    close_env();
    return true;
  }

  
}

bool LmdbChunk::isActive() {
  return env != NULL;
}

bool LmdbChunk::open_env() {

  if (env != NULL)
    return true;
  
  bool file_already_existed = rollingdbsupport::fileExists(this->db_fullpath.c_str());
  
  int rc;
  rc = mdb_env_create(&env);
  if (rc)
  {
    if (logger != NULL)
      LOG4CPLUS_WARN(*logger, "open_env mdb_txn_commit: (" << rc << ") " << mdb_strerror(rc) );
    
    mdb_env_close(env);
    env = NULL;
    return false;
  }
  
  // 1 megabyte * number of megabytes -- should be a multiple of 10
  size_t mapsize = 1048576 * MEGABYTES_PER_LMDB_CHUNK;
  
  rc = mdb_env_set_mapsize(env, mapsize);
  if (rc) 
  {
    if (logger != NULL)
      LOG4CPLUS_WARN(*logger, "open_env mdb_env_set_mapsize: (" << rc << ") " << mdb_strerror(rc) );
    
    mdb_env_close(env);
    env = NULL;
    return false;
  }
  
  rc = mdb_env_open(env, db_fullpath.c_str(), MDB_NOSUBDIR, 0664);
  if (rc) 
  {
    if (logger != NULL)
      LOG4CPLUS_WARN(*logger, "open_env mdb_env_open: (" << rc << ") " << mdb_strerror(rc) );
    
    mdb_env_close(env);
    env = NULL;
    return false;
  }


  return true;
}

void LmdbChunk::close_env() {

  if (env != NULL)
    mdb_env_close(env);
  
  env = NULL;
  
}

ReadStatus LmdbChunk::read_image(std::string name, std::vector<unsigned char>& image_bytes) {
  int rc;
  MDB_env *read_env;
          
  if (!rollingdbsupport::fileExists(db_fullpath.c_str()))
    return UNKNOWN_READ_FAILURE;
    
  rc = mdb_env_create(&read_env);
  if (rc)
    return UNKNOWN_READ_FAILURE;
  
  rc = mdb_env_open(read_env, db_fullpath.c_str(), MDB_NOSUBDIR, 0664);
  if (rc)
    return UNKNOWN_READ_FAILURE;
  
  MDB_val key, data;
  MDB_dbi dbi;
  MDB_txn *txn;
  //MDB_cursor *cursor;
  
  rc = mdb_txn_begin(read_env, NULL, MDB_RDONLY, &txn);
  if (rc)
  {
    mdb_env_close(read_env);
    return UNKNOWN_READ_FAILURE;
  }
  
  rc = mdb_dbi_open(txn, NULL, 0, &dbi);
  if (rc)
  {
    mdb_txn_abort(txn);
    mdb_env_close(read_env);
    return UNKNOWN_READ_FAILURE;
  }
  
  key.mv_size = strlen(name.c_str());
  key.mv_data = (void*) name.c_str();
  
//  cout << "Read Image key: " << (char*) key.mv_data << endl;
//  cout << "Read Image key size: " << key.mv_size << endl;
//  cout << "Searching in db: " << db_fullpath << endl;
//  cout << "Read Image key: " << (char*) key.mv_data << endl;
  
  rc = mdb_get(txn, dbi, &key, &data);
 
//  cout << "RC Status: " << rc << endl;
        
  if (rc)
  {
    mdb_txn_abort(txn);
    mdb_close(read_env, dbi);
    mdb_env_close(read_env);
    return IMAGE_NOT_FOUND;
  }
  
  image_bytes = vector<unsigned char>((unsigned char*)data.mv_data, ((unsigned char*) data.mv_data)+data.mv_size);
  //vector<unsigned char> input((unsigned char*)data.mv_data, ((unsigned char*) data.mv_data)+data.mv_size);
  mdb_txn_abort(txn);

  mdb_close(read_env, dbi);
  mdb_env_close(read_env);
  
  //cout << "Read Image size: " << data.mv_size << endl;
  
  
  
  return READ_SUCCESS;
}

WriteStatus LmdbChunk::write_image(std::vector<LmdbEntry> images) {

  int rc;
  MDB_val key, data;
  MDB_dbi dbi;
  MDB_txn *txn;
  
  if (!this->open_env())
  {
    LOG4CPLUS_WARN(*logger, "Unable to open image database (" << this->db_fullpath << ") for writing" );
    return DATABASE_NOT_ACTIVE;
  }

  //  sort the incoming images by key (alphabetically)
  std::sort( images.begin(), images.end(), imgCompare );
  
  
  //LOG4CPLUS_INFO(*logger, "Writing " << images.size() << " images to db" );
  
  rc = mdb_txn_begin(env, NULL, 0, &txn);
  if (rc) {
    if (logger != NULL)
      LOG4CPLUS_WARN(*logger, "mdb_txn_begin: (" << rc << ") " << mdb_strerror(rc) );
    return UNKNOWN_WRITE_FAILURE;
  }
  
  rc = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
  if (rc) {
    if (logger != NULL)
      LOG4CPLUS_WARN(*logger, "mdb_dbi_open: (" << rc << ") " << mdb_strerror(rc) );
    mdb_close(env, dbi);
    return UNKNOWN_WRITE_FAILURE;
  }
  
  for (size_t i = 0; i < images.size(); i++)
  {
    string name = images[i].key;
    
    if (parse_lmdb_epoch_time(name) < epoch_time_start)
    {
      if (logger != NULL)
        LOG4CPLUS_WARN(*logger, "Attempting to insert older image into latest database.  Ignoring" );
      continue;              
    }

    key.mv_size = strlen(name.c_str());
    key.mv_data = (void*) name.c_str();
    data.mv_size = images[i].image_bytes.size();
    data.mv_data = images[i].image_bytes.data();

    //cout << "Write Image key: " << (char*) key.mv_data << endl;
    //cout << "Write Image key size: " << key.mv_size << endl;
    //cout << "Write Image size: " << data.mv_size << endl;

    rc = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE);

    if (rc == MDB_MAP_FULL)
    {
      // Database is full, start the next one!
      //cout << "Database is full, let's move on to the next..." << endl;
      mdb_close(env, dbi);
      return DATABASE_FULL;
    }
    else if (rc == MDB_KEYEXIST)
    {
      if (logger != NULL)
        LOG4CPLUS_WARN(*logger, "Attempting to put duplicate key: " << name << " -- mdb_put: (" << rc << ") " << mdb_strerror(rc) );
      continue;
    }
    else if (rc) {
      if (logger != NULL)
        LOG4CPLUS_WARN(*logger, "mdb_put: (" << rc << ") " << mdb_strerror(rc) );
      mdb_close(env, dbi);
      return UNKNOWN_WRITE_FAILURE;
    }
  }
  
  rc = mdb_txn_commit(txn);
  if (rc == MDB_MAP_FULL)
  {
    // Database is full, start the next one!
    //cout << "Database is full, let's move on to the next..." << endl;
    mdb_close(env, dbi);
    return DATABASE_FULL;
  }
  else if (rc) {
    if (logger != NULL)
      LOG4CPLUS_WARN(*logger, "mdb_txn_commit: (" << rc << ") " << mdb_strerror(rc) );
    mdb_close(env, dbi);
    return UNKNOWN_WRITE_FAILURE;
  }
  
  mdb_close(env, dbi);
  
  return WRITE_SUCCESS;
}


int64_t LmdbChunk::parse_lmdb_epoch_time(std::string uuid) {
  
  int last_index = uuid.find_last_of("-");
  if (last_index < 0)
    return -1;
  
  std::string sub_file = uuid.substr(last_index);
  
  re2::RE2 re2_regex("[0-9]{13}");
  
  if (re2::RE2::PartialMatch(sub_file.c_str(), re2_regex))
  {
    string epoch_str = sub_file.substr(1, 13);
  
    char *pend;
    return strtoull(epoch_str.c_str(), &pend, 10);
  }
  
  return -1;
}

int64_t LmdbChunk::parse_database_epoch_time(std::string database_file_name) {
  char *pend;
  return strtoull(rollingdbsupport::filenameWithoutExtension(database_file_name).c_str(), &pend, 10);
}
