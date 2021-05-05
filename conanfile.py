from conans import ConanFile, CMake, tools

Options = [
    # Cafe
    ("CAFE_INCLUDE_TESTS", [True, False], False),

    # Cafe.Io
    ("CAFE_IO_INCLUDE_STREAMS", [True, False], True),
    ("CAFE_IO_INCLUDE_STREAM_HELPERS", [True, False], True),

    # Cafe.Io.Streams
    ("CAFE_IO_STREAMS_INCLUDE_FILE_STREAM", [True, False], True),
    ("CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING", [True, False], True),
]


class CafeIoConan(ConanFile):
    name = "Cafe.Io"
    version = "0.1"
    license = "MIT"
    author = "akemimadoka <chino@hotococoa.moe>"
    url = "https://github.com/akemimadoka/Cafe.Io"
    description = "A general purpose C++ library"
    topics = "C++"
    settings = "os", "compiler", "build_type", "arch"
    options = {opt[0]: opt[1] for opt in Options}
    default_options = {opt[0]: opt[2] for opt in Options}

    requires = "Cafe.ErrorHandling/0.1"

    generators = "cmake"

    exports_sources = "CMakeLists.txt", "CafeCommon*", "StreamHelpers*", "Streams*", "Test*"

    def requirements(self):
        if self.options.CAFE_INCLUDE_TESTS:
            self.requires("catch2/3.0.0@catchorg/stable", private=True)

    def configure_cmake(self):
        cmake = CMake(self)
        for opt in Options:
            cmake.definitions[opt[0]] = getattr(self.options, opt[0])
        cmake.configure()
        return cmake

    def build(self):
        with tools.vcvars(self.settings, filter_known_paths=False) if self.settings.compiler == 'Visual Studio' else tools.no_op():
            cmake = self.configure_cmake()
            cmake.build()

    def package(self):
        with tools.vcvars(self.settings, filter_known_paths=False) if self.settings.compiler == 'Visual Studio' else tools.no_op():
            cmake = self.configure_cmake()
            cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["Cafe.Io.Streams"]
