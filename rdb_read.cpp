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

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("rdb_read"));

int main( int argc, const char** argv )
{
  std::string out_file;
  std::string rollingdb_dir;
  std::string key;


  TCLAP::CmdLine cmd("RollingDB Read Data", ' ', "1.0");

  TCLAP::ValueArg<std::string> rollingDbDirArg("r","rollingdb_dir","Path of the rollingdb (default ./)",false, "./" ,"rollingdb_dir");
  TCLAP::ValueArg<std::string> outputFileArg("o","output_file","output location to write the file",true, "" ,"output_file");
  
  TCLAP::UnlabeledValueArg<std::string>  keyArg( "key", "Key for object in database", true, "", "key"  );


  try
  {
    cmd.add( outputFileArg );
    cmd.add( rollingDbDirArg );
    cmd.add( keyArg );
    

    cmd.parse( argc, argv );


    rollingdb_dir = rollingDbDirArg.getValue();
    out_file = outputFileArg.getValue();
    key = keyArg.getValue();
  }
  catch (TCLAP::ArgException &e)    // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return 1;
  }
  
  log4cplus::SharedAppenderPtr myAppender(new log4cplus::ConsoleAppender());
  myAppender->setName("rdb_read");
  logger.addAppender(myAppender);
  
  cout << "Loading RollingDB at location: " << rollingdb_dir << endl;
  
  const int MAX_DATABASE_SIZE_GB = 5;
  RollingDB rollingdb(rollingdb_dir, MAX_DATABASE_SIZE_GB, logger, true);
  
  std::vector<unsigned char> file_bytes;
  bool found_file = rollingdb.read_blob(key, file_bytes);
  
  if (!found_file)
  {
    cout << "Could not find key in database" << endl;
    return 1;
  }
  
  cout << "Writing data bytes to " << out_file << endl;
  ofstream fout;
  fout.open(out_file, ios::binary | ios::out);
  fout.write((char*) file_bytes.data(), file_bytes.size());
  fout.close();
  
  
  return 0;
}

