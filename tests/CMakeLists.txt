enable_testing()

ADD_EXECUTABLE( unittests  
  test_rollingdb.cpp 
)

TARGET_LINK_LIBRARIES(unittests

    rollingdb
    ${AlprSupport_LIBRARIES}
    ${log4cplus_LIBRARIES}
    ${WINDOWS_LIBS}
  )

add_test(unittests unittests)

