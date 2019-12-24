add_library(Cafe.Io.StreamHelpers INTERFACE)

target_include_directories(Cafe.Io.StreamHelpers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Io.StreamHelpers INTERFACE
    Cafe.Io.Streams Cafe.Environment)

install(TARGETS Cafe.Io.StreamHelpers
    EXPORT Io.StreamHelpers)

install(DIRECTORY src/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Io.StreamHelpers
    DESTINATION cmake/Io.StreamHelpers)
