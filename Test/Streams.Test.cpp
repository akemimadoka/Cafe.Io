#include <Cafe/Io/Streams/BufferedStream.h>
#include <Cafe/Io/Streams/FileStream.h>
#include <catch2/catch.hpp>

using namespace Cafe;
using namespace Io;
using namespace Encoding::StringLiterals;

TEST_CASE("Cafe.Io.Streams", "[Io][Streams]")
{
#if CAFE_IO_STREAMS_INCLUDE_FILE_STREAM
	SECTION("FileStreams")
	{
		constexpr const char Data[] = "Some Text";

		FileOutputStream tmpFile{ u"Temp.txt"_sv };
		const auto writtenSize = tmpFile.WriteBytes(gsl::as_bytes(gsl::make_span(Data)));
		REQUIRE(writtenSize == 10);
		tmpFile.Close();

		FileInputStream tmpFile2{ u"Temp.txt"_sv };
		std::byte buffer[10];
		const auto readSize = tmpFile2.ReadBytes(gsl::make_span(buffer));
		REQUIRE(readSize == 10);

		REQUIRE(std::memcmp(Data, buffer, 10) == 0);
	}

	SECTION("BufferedStreams")
	{
	}
#endif
}
