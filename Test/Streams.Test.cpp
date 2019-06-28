#include <Cafe/Io/Streams/BufferedStream.h>
#include <Cafe/Io/Streams/FileStream.h>
#include <Cafe/Io/Streams/MemoryStream.h>
#include <catch2/catch.hpp>

using namespace Cafe;
using namespace Io;
using namespace Encoding::StringLiterals;

constexpr const char Data[] = "Some Text";

TEST_CASE("Cafe.Io.Streams", "[Io][Streams]")
{
#if CAFE_IO_STREAMS_INCLUDE_FILE_STREAM
	SECTION("FileStreams")
	{
		FileOutputStream tmpFile{ u"Temp.txt"_sv };
		const auto writtenSize = tmpFile.WriteBytes(gsl::as_bytes(gsl::make_span(Data)));
		REQUIRE(writtenSize == 10);
		tmpFile.Close();

		FileInputStream tmpFile2{ u"Temp.txt"_sv };
		std::byte buffer[10];
		const auto readSize = tmpFile2.ReadBytes(gsl::make_span(buffer));
		REQUIRE(readSize == 10);

		REQUIRE(std::memcmp(Data, buffer, 10) == 0);

		auto mappedStream = tmpFile2.MapToMemory();
		const auto storage = mappedStream.GetStorage();

		REQUIRE(std::memcmp(Data, storage.data(), 10) == 0);

		auto stdOutStream = FileOutputStream::CreateStdOutStream();
		stdOutStream.WriteBytes(gsl::as_bytes(gsl::make_span("Hello?\n")));
	}
#endif
	SECTION("MemoryStreams")
	{
		{
			MemoryStream stream;
			const auto writtenSize = stream.WriteBytes(gsl::as_bytes(gsl::make_span(Data)));
			REQUIRE(writtenSize == 10);
			REQUIRE(stream.GetPosition() == 10);
			REQUIRE(stream.GetInternalStorage().size() == 10);

			stream.SeekFromBegin(0);
			REQUIRE(stream.GetPosition() == 0);

			stream.Seek(SeekOrigin::Current, 4);
			REQUIRE(stream.GetPosition() == 4);

			stream.Seek(SeekOrigin::Current, -3);
			REQUIRE(stream.GetPosition() == 1);

			stream.Seek(SeekOrigin::Begin, 6);
			REQUIRE(stream.GetPosition() == 6);

			stream.Seek(SeekOrigin::End, -2);
			REQUIRE(stream.GetPosition() == 8);

			stream.SeekFromBegin(0);

			std::byte buffer[10];
			const auto readSize = stream.ReadBytes(gsl::make_span(buffer));
			REQUIRE(readSize == 10);
			REQUIRE(stream.GetPosition() == 10);

			REQUIRE(std::memcmp(Data, buffer, 10) == 0);
		}

		{
			ExternalMemoryInputStream stream{ gsl::as_bytes(gsl::make_span(Data)) };

			std::byte buffer[10];
			const auto readSize = stream.ReadBytes(gsl::make_span(buffer));
			REQUIRE(readSize == 10);
			REQUIRE(stream.GetPosition() == 10);

			REQUIRE(std::memcmp(Data, buffer, 10) == 0);
		}

		{
			std::byte buffer[10];

			ExternalMemoryOutputStream stream{ gsl::make_span(buffer) };

			const auto writtenSize = stream.WriteBytes(gsl::as_bytes(gsl::make_span(Data)));
			REQUIRE(writtenSize == 10);
			REQUIRE(stream.GetPosition() == 10);

			REQUIRE(std::memcmp(Data, buffer, 10) == 0);
		}
	}

	SECTION("BufferedStreams")
	{
		constexpr const std::uint8_t Data[]{ 0x00, 0x01, 0x02, 0x03 };

		MemoryStream stream;

		{
			BufferedOutputStream bufferedStream{ &stream };
			const auto writtenSize = bufferedStream.WriteBytes(gsl::as_bytes(gsl::make_span(Data)));

			REQUIRE(writtenSize == 4);
			REQUIRE(stream.GetPosition() == 0);

			bufferedStream.Flush();

			REQUIRE(stream.GetPosition() == 4);
		}

		stream.SeekFromBegin(0);

		{
			std::byte buffer[4];

			BufferedInputStream bufferedStream{ &stream };
			const auto readSize = bufferedStream.ReadBytes(gsl::make_span(buffer));

			REQUIRE(readSize == 4);

			REQUIRE(std::memcmp(Data, buffer, 4) == 0);
		}
	}
}
