#include <Cafe/Io/Streams/BufferedStream.h>
#include <Cafe/Io/Streams/FileStream.h>
#include <Cafe/Io/Streams/MemoryStream.h>
#include <catch2/catch_all.hpp>
#include <cstring>

using namespace Cafe;
using namespace Io;
using namespace Encoding::StringLiterals;

constexpr const char Data[] = "Some Text";

TEST_CASE("Cafe.Io.Streams", "[Io][Streams]")
{
#if CAFE_IO_STREAMS_INCLUDE_FILE_STREAM
	SECTION("FileStreams")
	{
#ifdef _WIN32
		const auto fileName = u"Temp.txt"_sv;
#else
		const auto fileName = u8"Temp.txt"_sv;
#endif
		FileOutputStream tmpFile{ fileName };
		const auto writtenSize = tmpFile.WriteBytes(std::as_bytes(std::span(Data)));
		REQUIRE(writtenSize == 10);
		tmpFile.Close();

		FileInputStream tmpFile2{ fileName };
		std::byte buffer[10];
		const auto readSize = tmpFile2.ReadBytes(std::span(buffer));
		REQUIRE(readSize == 10);

		REQUIRE(std::memcmp(Data, buffer, 10) == 0);

		auto mappedStream = tmpFile2.MapToMemory();
		const auto storage = mappedStream.GetStorage();

		REQUIRE(std::memcmp(Data, storage.data(), 10) == 0);

		auto stdOutStream = FileOutputStream::CreateStdOutStream();
		stdOutStream.WriteBytes(std::as_bytes(std::span("Hello?\n")));
	}
#endif
	SECTION("MemoryStreams")
	{
		{
			MemoryStream stream;
			const auto writtenSize = stream.WriteBytes(std::as_bytes(std::span(Data)));
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
			const auto readSize = stream.ReadBytes(std::span(buffer));
			REQUIRE(readSize == 10);
			REQUIRE(stream.GetPosition() == 10);

			REQUIRE(std::memcmp(Data, buffer, 10) == 0);
		}

		{
			ExternalMemoryInputStream stream{ std::as_bytes(std::span(Data)) };

			std::byte buffer[10];
			const auto readSize = stream.ReadBytes(std::span(buffer));
			REQUIRE(readSize == 10);
			REQUIRE(stream.GetPosition() == 10);

			REQUIRE(std::memcmp(Data, buffer, 10) == 0);
		}

		{
			std::byte buffer[10];

			ExternalMemoryOutputStream stream{ std::span(buffer) };

			const auto writtenSize = stream.WriteBytes(std::as_bytes(std::span(Data)));
			REQUIRE(writtenSize == 10);
			REQUIRE(stream.GetPosition() == 10);

			REQUIRE(std::memcmp(Data, buffer, 10) == 0);
		}
	}

	SECTION("BufferedStreams")
	{
		constexpr const std::uint8_t Data[]{ 0x00, 0x01, 0x02, 0x03 };

		MemoryStream stream;

		[[maybe_unused]] const auto inputStream = static_cast<InputStream*>(&stream);
		[[maybe_unused]] const auto outputStream = static_cast<OutputStream*>(&stream);
		[[maybe_unused]] const auto inputOutputStream = static_cast<InputOutputStream*>(&stream);
		[[maybe_unused]] const auto seekableInputStream =
		    static_cast<SeekableStream<InputStream>*>(&stream);
		[[maybe_unused]] const auto seekableOutputStream =
		    static_cast<SeekableStream<OutputStream>*>(&stream);
		[[maybe_unused]] const auto seekableInputOutputStream =
		    static_cast<SeekableStream<InputOutputStream>*>(&stream);
		[[maybe_unused]] const auto seekableStreamBase =
		    static_cast<SeekableStream<Stream>*>(&stream);

		{
			BufferedOutputStream bufferedStream{ &stream };
			const auto writtenSize = bufferedStream.WriteBytes(std::as_bytes(std::span(Data)));

			REQUIRE(writtenSize == 4);
			REQUIRE(stream.GetPosition() == 0);

			bufferedStream.Flush();

			REQUIRE(stream.GetPosition() == 4);
		}

		stream.SeekFromBegin(0);

		{
			std::byte buffer[4];

			BufferedInputStream bufferedStream{ &stream };
			const auto readSize = bufferedStream.ReadBytes(std::span(buffer));

			REQUIRE(readSize == 4);

			REQUIRE(std::memcmp(Data, buffer, 4) == 0);
		}
	}
}
