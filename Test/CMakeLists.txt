set(SOURCE_FILES Main.cpp)

if(CAFE_IO_INCLUDE_STREAMS)
    list(APPEND SOURCE_FILES Streams.Test.cpp)
endif()

if(CAFE_IO_INCLUDE_STREAM_HELPERS)
    list(APPEND SOURCE_FILES StreamHelpers.Test.cpp)
endif()

add_executable(Cafe.Io.Test ${SOURCE_FILES})

target_link_libraries(Cafe.Io.Test PRIVATE
    CONAN_PKG::catch2)

if(CAFE_IO_INCLUDE_STREAMS)
    target_link_libraries(Cafe.Io.Test PRIVATE
        Cafe.Io.Streams)
endif()

if(CAFE_IO_INCLUDE_STREAM_HELPERS)
    target_link_libraries(Cafe.Io.Test PRIVATE
        Cafe.Io.StreamHelpers)
endif()

catch_discover_tests(Cafe.Io.Test)
