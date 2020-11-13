#include <Cafe/ErrorHandling/ErrorHandling.h>
#include <Cafe/Io/Streams/BufferedStream.h>
#include <cstring>

using namespace Cafe;
using namespace Io;

BufferedInputStream::BufferedInputStream(InputStream* stream, std::size_t maxBufferSize)
    : m_UnderlyingStream{ stream },
      m_LastReadBufferPosition(-1), m_Buffer{ std::make_unique<std::byte[]>(maxBufferSize) },
      m_MaxBufferSize{ maxBufferSize }, m_ReadSize{}, m_CurrentPosition{}
{
}

BufferedInputStream::BufferedInputStream(BufferedInputStream&& other) noexcept
    : m_UnderlyingStream{ std::exchange(other.m_UnderlyingStream, nullptr) },
      m_LastReadBufferPosition{ other.m_LastReadBufferPosition },
      m_Buffer{ std::move(other.m_Buffer) }, m_MaxBufferSize{ other.m_MaxBufferSize },
      m_ReadSize{ other.m_ReadSize }, m_CurrentPosition{ other.m_CurrentPosition }
{
}

BufferedInputStream::~BufferedInputStream()
{
	BufferedInputStream::Close();
}

BufferedInputStream& BufferedInputStream::operator=(BufferedInputStream&& other) noexcept
{
	m_UnderlyingStream = std::exchange(other.m_UnderlyingStream, nullptr);
	m_LastReadBufferPosition = other.m_LastReadBufferPosition;
	m_Buffer = std::move(other.m_Buffer);
	m_MaxBufferSize = other.m_MaxBufferSize;
	m_ReadSize = other.m_ReadSize;
	m_CurrentPosition = other.m_CurrentPosition;

	return *this;
}

void BufferedInputStream::Close()
{
	if (const auto seekableStream = dynamic_cast<SeekableStream<InputStream>*>(m_UnderlyingStream);
	    seekableStream && m_LastReadBufferPosition != std::size_t(-1))
	{
		seekableStream->SeekFromBegin(m_LastReadBufferPosition + m_CurrentPosition);
	}

	m_UnderlyingStream = nullptr;
	m_Buffer.reset();
}

std::size_t BufferedInputStream::GetAvailableBytes()
{
	return m_ReadSize - m_CurrentPosition + m_UnderlyingStream->GetAvailableBytes();
}

std::size_t BufferedInputStream::ReadBytes(std::span<std::byte> const& buffer)
{
	const auto readSizeFromBuffer =
	    std::min(m_ReadSize - m_CurrentPosition, static_cast<std::size_t>(buffer.size()));
	std::memcpy(buffer.data(), &m_Buffer[m_CurrentPosition], readSizeFromBuffer);
	m_CurrentPosition += readSizeFromBuffer;
	auto readSize = readSizeFromBuffer;
	if (readSizeFromBuffer != buffer.size())
	{
		readSize += m_UnderlyingStream->ReadBytes(buffer.subspan(readSizeFromBuffer));
		FlushBuffer(false);
	}
	else if (m_CurrentPosition == m_ReadSize)
	{
		FlushBuffer();
	}

	return readSize;
}

std::size_t BufferedInputStream::Skip(std::size_t n)
{
	if (m_CurrentPosition + n < m_ReadSize)
	{
		m_CurrentPosition += n;
		return n;
	}

	const auto skippedSize = m_ReadSize - m_CurrentPosition + m_UnderlyingStream->Skip(n);
	FlushBuffer(false);
	return skippedSize;
}

std::size_t BufferedInputStream::GetPosition() const
{
	if (const auto seekableStream = dynamic_cast<SeekableStream<InputStream>*>(m_UnderlyingStream))
	{
		if (m_LastReadBufferPosition == std::size_t(-1))
		{
			return seekableStream->GetPosition();
		}
		return m_LastReadBufferPosition + m_CurrentPosition;
	}

	CAFE_THROW(IoException, CAFE_UTF8_SV("Underlying stream is not seekable."));
}

void BufferedInputStream::SeekFromBegin(std::size_t pos)
{
	if (const auto seekableStream = dynamic_cast<SeekableStream<InputStream>*>(m_UnderlyingStream))
	{
		seekableStream->SeekFromBegin(pos);
		FlushBuffer(false);
	}
	else
	{
		CAFE_THROW(IoException, CAFE_UTF8_SV("Underlying stream is not seekable."));
	}
}

void BufferedInputStream::Seek(SeekOrigin origin, std::ptrdiff_t diff)
{
	if (origin == SeekOrigin::Current && m_CurrentPosition <= diff &&
	    diff <= m_ReadSize - m_CurrentPosition)
	{
		m_CurrentPosition += diff;
		return;
	}

	if (const auto seekableStream = dynamic_cast<SeekableStream<InputStream>*>(m_UnderlyingStream))
	{
		if (origin == SeekOrigin::Current)
		{
			diff += seekableStream->GetPosition() - (m_ReadSize - m_CurrentPosition);
		}
		seekableStream->Seek(origin, diff);
		FlushBuffer(false);
	}
	else
	{
		CAFE_THROW(IoException, CAFE_UTF8_SV("Underlying stream is not seekable."));
	}
}

std::size_t BufferedInputStream::GetTotalSize()
{
	if (const auto seekableStream = dynamic_cast<SeekableStream<InputStream>*>(m_UnderlyingStream))
	{
		return seekableStream->GetTotalSize();
	}

	CAFE_THROW(IoException, CAFE_UTF8_SV("Underlying stream is not seekable."));
}

std::size_t BufferedInputStream::GetMaxBufferSize() const noexcept
{
	return m_MaxBufferSize;
}

std::size_t BufferedInputStream::GetBufferSize() const noexcept
{
	return m_ReadSize;
}

std::size_t BufferedInputStream::GetAvailableBufferSize() const noexcept
{
	return m_ReadSize - m_CurrentPosition;
}

void BufferedInputStream::FlushBuffer(bool keep, std::size_t needSize)
{
	const auto keepSize = keep ? m_ReadSize - m_CurrentPosition : 0;
	std::memmove(m_Buffer.get(), &m_Buffer[m_CurrentPosition], keepSize);
	if (const auto seekableStream = dynamic_cast<SeekableStream<InputStream>*>(m_UnderlyingStream))
	{
		m_LastReadBufferPosition = seekableStream->GetPosition() - keepSize;
	}
	if (m_MaxBufferSize != keepSize && needSize)
	{
		const auto readSize = m_UnderlyingStream->ReadAvailableBytes(std::span(
		    &m_Buffer[keep ? m_CurrentPosition : 0], std::min(m_MaxBufferSize - keepSize, needSize)));
		m_ReadSize = keepSize + readSize;
	}
	else
	{
		m_ReadSize = keepSize;
	}

	m_CurrentPosition = 0;
}

void BufferedInputStream::FillBuffer(bool keep, std::size_t needSize)
{
	const auto keepSize = keep ? m_ReadSize - m_CurrentPosition : 0;
	std::memmove(m_Buffer.get(), &m_Buffer[m_CurrentPosition], keepSize);
	if (const auto seekableStream = dynamic_cast<SeekableStream<InputStream>*>(m_UnderlyingStream))
	{
		m_LastReadBufferPosition = seekableStream->GetPosition() - keepSize;
	}
	if (m_MaxBufferSize != keepSize && needSize)
	{
		const auto readSize = m_UnderlyingStream->ReadBytes(std::span(
		    &m_Buffer[keep ? m_CurrentPosition : 0], std::min(m_MaxBufferSize - keepSize, needSize)));
		m_ReadSize = keepSize + readSize;
	}
	else
	{
		m_ReadSize = keepSize;
	}

	m_CurrentPosition = 0;
}

std::optional<std::byte> BufferedInputStream::PeekByte()
{
	if (!m_MaxBufferSize)
	{
		CAFE_THROW(IoException, CAFE_UTF8_SV("BufferSize is 0, PeekByte is not implementable."));
	}

	if (m_CurrentPosition == m_ReadSize)
	{
		FillBuffer(false, 1);

		// 流已到结尾
		if (m_CurrentPosition == m_ReadSize)
		{
			return {};
		}
	}

	return m_Buffer[m_CurrentPosition];
}

std::size_t BufferedInputStream::PeekBytes(std::span<std::byte> const& buffer)
{
	FillBuffer(true, buffer.size());

	const auto readSize = std::min(m_ReadSize, static_cast<std::size_t>(buffer.size()));
	std::memcpy(buffer.data(), m_Buffer.get(), readSize);
	return readSize;
}

BufferedOutputStream::BufferedOutputStream(OutputStream* stream, std::size_t bufferSize)
    : m_UnderlyingStream{ stream }, m_Buffer{ std::make_unique<std::byte[]>(bufferSize) },
      m_BufferSize{ bufferSize }, m_CurrentPosition{}
{
}

BufferedOutputStream::BufferedOutputStream(BufferedOutputStream&& other) noexcept
    : m_UnderlyingStream{ std::exchange(other.m_UnderlyingStream, nullptr) }, m_Buffer{ std::move(
	                                                                                other.m_Buffer) },
      m_BufferSize{ other.m_BufferSize }, m_CurrentPosition{ other.m_CurrentPosition }
{
}

BufferedOutputStream::~BufferedOutputStream()
{
	BufferedOutputStream::Close();
}

BufferedOutputStream& BufferedOutputStream::operator=(BufferedOutputStream&& other) noexcept
{
	Flush();

	m_UnderlyingStream = std::exchange(other.m_UnderlyingStream, nullptr);
	m_Buffer = std::move(other.m_Buffer);
	m_BufferSize = other.m_BufferSize;
	m_CurrentPosition = other.m_CurrentPosition;

	return *this;
}

void BufferedOutputStream::Close()
{
	if (m_UnderlyingStream)
	{
		Flush();

		m_UnderlyingStream = nullptr;
		m_Buffer.reset();
	}
}

std::size_t BufferedOutputStream::WriteBytes(std::span<const std::byte> const& buffer)
{
	const auto writeSizeToBuffer =
	    std::min(m_BufferSize - m_CurrentPosition, static_cast<std::size_t>(buffer.size()));
	std::memcpy(&m_Buffer[m_CurrentPosition], buffer.data(), writeSizeToBuffer);
	m_CurrentPosition += writeSizeToBuffer;
	auto writtenSize = writeSizeToBuffer;
	if (writeSizeToBuffer != buffer.size())
	{
		Flush();
		writtenSize += m_UnderlyingStream->WriteBytes(buffer.subspan(writeSizeToBuffer));
	}
	else if (m_CurrentPosition == m_BufferSize)
	{
		Flush();
	}

	return writtenSize;
}

void BufferedOutputStream::Flush()
{
	if (m_CurrentPosition)
	{
		m_UnderlyingStream->WriteBytes(std::span(m_Buffer.get(), m_CurrentPosition));
		m_CurrentPosition = 0;
	}
}
