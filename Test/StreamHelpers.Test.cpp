#include <Cafe/Io/StreamHelpers/BinaryReader.h>
#include <Cafe/Io/StreamHelpers/BinaryWriter.h>
#include <Cafe/Io/Streams/MemoryStream.h>
#include <catch2/catch.hpp>

using namespace Cafe;
using namespace Io;

namespace
{
	constexpr std::uint8_t Data[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		                                0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
}

TEST_CASE("Cafe.Io.StreamHelpers", "[Io][StreamHelpers]")
{
	SECTION("Test BinaryReader")
	{
		ExternalMemoryInputStream stream{ std::as_bytes(std::span(Data)) };
		BinaryReader<> reader{ &stream, std::endian::little };

		const auto u8 = reader.Read<std::uint8_t>();
		REQUIRE(u8);
		REQUIRE(*u8 == 0x01);

		const auto u16 = reader.Read<std::uint16_t>();
		REQUIRE(u16);
		REQUIRE(*u16 == (std::endian::native == std::endian::little ? 0x0302 : 0x0203));

		const auto u32 = reader.Read<std::uint32_t>();
		REQUIRE(u32);
		REQUIRE(*u32 == (std::endian::native == std::endian::little ? 0x07060504 : 0x04050607));

		const auto u64 = reader.Read<std::uint64_t>();
		REQUIRE(u64);
		REQUIRE(*u64 ==
		        (std::endian::native == std::endian::little ? 0x0f0e0d0c0b0a0908 : 0x08090a0b0c0d0e0f));

		REQUIRE(stream.GetPosition() == std::size(Data));

		// 未测试大小不为 1 2 4 8 的标量类型及所有浮点类型
	}

	SECTION("Test BinaryWriter")
	{
		std::byte buffer[15];
		ExternalMemoryOutputStream stream{ std::span(buffer) };
		BinaryWriter<> writer{ &stream, std::endian::little };

		writer.Write(std::uint8_t{ 0x01 });
		writer.Write(std::uint16_t{ 0x0302 });
		writer.Write(std::uint32_t{ 0x07060504 });
		writer.Write(std::uint64_t{ 0x0f0e0d0c0b0a0908 });

		REQUIRE(stream.GetPosition() == std::size(buffer));

		if constexpr (std::endian::native == std::endian::little)
		{
			REQUIRE(std::memcmp(buffer, Data, 15) == 0);
		}
		else
		{
			constexpr std::uint8_t BigData[] = { 0x01, 0x03, 0x02, 0x07, 0x06, 0x05, 0x04, 0x0f,
				                                   0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08 };
			REQUIRE(std::memcmp(buffer, BigData, 15) == 0);
		}

		// 未测试大小不为 1 2 4 8 的标量类型及所有浮点类型
	}
}
