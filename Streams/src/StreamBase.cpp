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

std::size_t InputStream::ReadBytes(gsl::span<std::byte> const& buffer)
{
	const auto readSize = static_cast<std::size_t>(buffer.size());
	for (std::size_t i = 0; i < readSize; ++i)
	{
		const auto readByte = ReadByte();
		if (readByte.has_value())
		{
			buffer[i] = readByte.value();
		}
		else
		{
			return i;
		}
	}

	return readSize;
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
		    ReadBytes(gsl::make_span(buffer, std::min(sizeof buffer, remainedBytes)));
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

void OutputStream::Flush()
{
}

InputOutputStream::~InputOutputStream()
{
}

SeekableStreamBase::~SeekableStreamBase()
{
}

void SeekableStreamBase::SeekFromBegin(std::size_t pos)
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

std::size_t SeekableStreamBase::GetTotalSize()
{
	const auto curPos = GetPosition();
	CAFE_SCOPE_EXIT
	{
		SeekFromBegin(curPos);
	};

	Seek(SeekOrigin::End, 0);
	return GetPosition();
}

template class SeekableStream<InputStream>;
template class SeekableStream<OutputStream>;
template class SeekableStream<InputOutputStream>;
