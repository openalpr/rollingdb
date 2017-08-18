/* 
 * File:   RollingDB.cpp
 * Author: mhill
 * 
 * Created on March 31, 2016, 9:37 PM
 */

#include "rollingdb.h"

#include "rollingdb_impl.h"


using namespace std;


RollingDB::RollingDB(std::string chunk_directory, int max_size_gb, int jpeg_quality, log4cplus::Logger logger, bool read_only)
{
  this->impl = new RollingDBImpl(chunk_directory, max_size_gb, jpeg_quality, logger, read_only);
}


RollingDB::~RollingDB() {

  delete impl;
  
}





bool RollingDB::read_image(std::string name, std::vector<unsigned char>& image_bytes) {
  return impl->read_image(name, image_bytes);
}

}

void RollingDB::reload_from_disk() {
  impl->reload_from_disk();
}

void RollingDB::write_image(std::string name, std::vector<unsigned char>& image_bytes) {
  impl->write_image(name, image_bytes);
}

