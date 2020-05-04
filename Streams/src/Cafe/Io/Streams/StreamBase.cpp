#include <Cafe/Io/Streams/StreamBase.h>

using namespace Cafe::Io;

namespace
{
	constexpr std::size_t DefaultSkipBufferSize = 1024;
}

Stream::~Stream()
{
}

void Stream::Close()
{
}

InputStream::~InputStream()
{
}

std::optional<std::byte> InputStream::ReadByte()
{
	std::byte value;
	if (!ReadBytes(gsl::span(&value, 1)))
	{
		return {};
	}

	return value;
}

std::size_t InputStream::ReadAvailableBytes(gsl::span<std::byte> const& buffer)
{
	const auto availableSize = GetAvailableBytes();
	const auto readSize = std::min(availableSize, static_cast<std::size_t>(buffer.size()));
	if (readSize)
	{
		return ReadBytes(buffer.subspan(0, readSize));
	}

	return 0;
}

std::size_t InputStream::Skip(std::size_t n)
{
	auto remainedBytes = n;
	std::byte buffer[DefaultSkipBufferSize];
	while (remainedBytes)
	{
		const auto readBytes =
		    ReadBytes(gsl::span(buffer, std::min(sizeof buffer, remainedBytes)));
		if (!readBytes)
		{
			// 出现错误，返回
			break;
		}

		remainedBytes -= readBytes;
	}

	return n - remainedBytes;
}

OutputStream::~OutputStream()
{
}

bool OutputStream::WriteByte(std::byte value)
{
	return WriteBytes(gsl::span(&value, 1));
}

void OutputStream::Flush()
{
}

InputOutputStream::~InputOutputStream()
{
}

SeekableStream<Stream>::~SeekableStream()
{
}

void SeekableStream<Stream>::SeekFromBegin(std::size_t pos)
{
	if (pos <= static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()))
	{
		Seek(SeekOrigin::Begin, static_cast<std::ptrdiff_t>(pos));
	}
	else
	{
		Seek(SeekOrigin::Begin, std::numeric_limits<std::ptrdiff_t>::max());
		Seek(SeekOrigin::Current,
		     static_cast<std::ptrdiff_t>(pos - std::numeric_limits<std::ptrdiff_t>::max()));
	}
}

std::size_t SeekableStream<Stream>::GetTotalSize()
{
	const auto curPos = GetPosition();
	CAFE_SCOPE_EXIT
	{
		SeekFromBegin(curPos);
	};

	Seek(SeekOrigin::End, 0);
	return GetPosition();
}

SeekableStream<InputStream>::~SeekableStream()
{
}

std::size_t SeekableStream<InputStream>::Skip(std::size_t n)
{
	const auto skippingBytes = std::min(n, GetAvailableBytes());

	if (skippingBytes > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()))
		[[unlikely]]
		{
			Seek(SeekOrigin::Current, std::numeric_limits<std::ptrdiff_t>::max());
			Seek(SeekOrigin::Current,
			     static_cast<std::ptrdiff_t>(
			         skippingBytes -
			         static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max())));
		}
	else
	{
		Seek(SeekOrigin::Current, static_cast<std::ptrdiff_t>(skippingBytes));
	}

	return skippingBytes;
}

SeekableStream<OutputStream>::~SeekableStream()
{
}

SeekableStream<InputOutputStream>::~SeekableStream()
{
}
