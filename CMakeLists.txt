set(CAFE_IO_INCLUDE_STREAMS ON CACHE BOOL "Include Cafe.Io.Streams")
set(CAFE_IO_INCLUDE_TEST ON CACHE BOOL "Include Cafe.Io.Test")

if(CAFE_IO_INCLUDE_STREAMS)
    add_subdirectory(Streams)
endif()

if(CAFE_INCLUDE_TESTS AND CAFE_IO_INCLUDE_TEST)
    add_subdirectory(Test)
endif()
