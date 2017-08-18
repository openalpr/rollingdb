/* 
 * Copyright 2017, OpenALPR Technology, Inc.  
 * All rights reserved
 * This file is part of the RollingDB library
 * RollingDB is licensed under LGPL
 */

#include <cstdlib>
#include <tclap/CmdLine.h>
#include "src/system_utils.h"
#include "src/rollingdb.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/consoleappender.h>

using namespace std;

/*
 * 
 */

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("rdb_write"));

int main( int argc, const char** argv )
{
  std::vector<std::string> filenames;
  std::string rollingdb_dir;


  TCLAP::CmdLine cmd("RollingDB Write Data", ' ', "1.0");

  TCLAP::UnlabeledMultiArg<std::string>  fileArg( "files", "Binary files to add to the database", true, "", "files"  );

  TCLAP::ValueArg<std::string> rollingDbDirArg("r","rollingdb_dir","Path of the rollingdb (default ./)",false, "./" ,"rollingdb_dir");

  try
  {
    cmd.add( fileArg );
    cmd.add( rollingDbDirArg );
    

    cmd.parse( argc, argv );


    filenames = fileArg.getValue();
    rollingdb_dir = rollingDbDirArg.getValue();

  }
  catch (TCLAP::ArgException &e)    // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return 1;
  }
  
  log4cplus::SharedAppenderPtr myAppender(new log4cplus::ConsoleAppender());
  myAppender->setName("rdb_write");
  logger.addAppender(myAppender);
  
  cout << "Loading/Initializing RollingDB at location: " << rollingdb_dir << endl;
  
  const int MAX_DATABASE_SIZE_GB = 5;
  RollingDB rollingdb(rollingdb_dir, MAX_DATABASE_SIZE_GB, logger, false);
  
  for (uint32_t i = 0; i < filenames.size(); i++)
  {
    if (rollingdbsupport::fileExists(filenames[i].c_str()) && !rollingdbsupport::DirectoryExists(filenames[i].c_str()))
    {
      uint64_t epoch_time = rollingdbsupport::getEpochTimeMs();
      stringstream key_name;
      key_name << filenames[i] << "-" << rollingdbsupport::getEpochTimeMs() ;
      
      cout << "Writing " << filenames[i] << " to RollingDB with key: " << key_name.str() << endl;
      
      std::vector<unsigned char> file_bytes = rollingdbsupport::ReadAllBytes(filenames[i].c_str());
      rollingdb.write_blob(filenames[i], epoch_time, file_bytes);
    }
  }
  
  // Wait for asynchronous write thread to complete
  while (rollingdb.get_write_buffer_size() > 0)
    rollingdbsupport::sleep_ms(100);
  
  return 0;
}

