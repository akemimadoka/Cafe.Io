set(CAFE_IO_INCLUDE_STREAMS ON CACHE BOOL "Include Cafe.Io.Streams")

if(CAFE_IO_INCLUDE_STREAMS)
    add_subdirectory(Streams)
endif()
