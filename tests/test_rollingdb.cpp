
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"
#include "chunkcollection.h"
#include "system_utils.h"
#include <string>
#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>

using namespace std;

TEST_CASE( "Test Chunk Collection", "[image archive]" ) {

  if (rollingdbsupport::DirectoryExists("/tmp/chunktest/image_db/"))
  {
    std::vector<string> files = rollingdbsupport::getFilesInDir("/tmp/chunktest/image_db/");
    for (int i = 0; i < files.size(); i++)
    {
      stringstream ss;
      ss << "/tmp/chunktest/image_db/" << files[i];
      remove(ss.str().c_str());
    }
  }
  
  // Test inserts
  log4cplus::SharedAppenderPtr myAppender(new log4cplus::ConsoleAppender());
  log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("imgtest"));
  logger.addAppender(myAppender);
  
  ChunkCollection chunkcol("/tmp/chunktest/", 50, logger, false);
  
  REQUIRE( chunkcol.size() == 0 );
  
  chunkcol.new_chunk(1000);
  
  REQUIRE( chunkcol.size() == 1 );
  
  for (int i = 0; i < 60; i++)
    chunkcol.new_chunk(1000 + (i * 10) );
  
  REQUIRE( chunkcol.size() == 50 );
  
  // Test searches
  LmdbChunk out_chunk;
  REQUIRE(chunkcol.get_chunk_path(999, out_chunk) == false);
  
  REQUIRE (chunkcol.get_chunk_path(1025, out_chunk) == false);
  
  REQUIRE (chunkcol.get_chunk_path(1095, out_chunk) == false);
  
  REQUIRE (chunkcol.get_chunk_path(1175, out_chunk) == true);
  REQUIRE (out_chunk.db_fullpath == "/tmp/chunktest/image_db/1170.mdb");
  
  ChunkCollection chunkcol2("/tmp/chunktest/", 50, logger, true);
  
  LmdbChunk chunk("/tmp/chunktest/image_db/1170.mdb");
  REQUIRE ( chunk.epoch_time_start == 1170);

}
