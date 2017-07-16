/* 
 * File:   RollingDB.h
 * Author: mhill
 *
 * Created on March 31, 2016, 9:37 PM
 */

#ifndef OPENALPR_ROLLINGDB_H
#define	OPENALPR_ROLLINGDB_H


#include <string>
#include <opencv2/opencv.hpp>

#include <log4cplus/logger.h>


class RollingDBImpl;
class RollingDB {
public:
  RollingDB(std::string chunk_directory, int max_size_gb, int jpeg_quality, log4cplus::Logger logger, bool read_only = false);
  virtual ~RollingDB();
  
  void write_image(std::string name, cv::Mat image);
  void write_image(std::string name, std::vector<uchar>& image_bytes);
  
  bool read_image(std::string name, cv::Mat& output_image);
  bool read_image(std::string name, std::vector<uchar>& image_bytes);
  
  bool active;
  
  void reload_from_disk();
  
private:
  RollingDBImpl* impl;
  
};

#endif	/* OPENALPR_ROLLINGDB_H */

