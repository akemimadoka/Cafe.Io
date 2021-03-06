#include <Cafe/Io/Streams/MemoryStream.h>
#include <cstring>

using namespace Cafe;
using namespace Io;

MemoryStream::MemoryStream() : m_CurrentPosition{}
{
}

MemoryStream::MemoryStream(std::span<const std::byte> const& initialContent)
    : m_Storage(initialContent.begin(), initialContent.end()), m_CurrentPosition{}
{
}

MemoryStream::MemoryStream(std::vector<std::byte>&& initialStorage)
    : m_Storage(std::move(initialStorage))
{
}

MemoryStream::~MemoryStream()
{
}

void MemoryStream::Close()
{
	m_Storage.clear();
	m_Storage.shrink_to_fit();
	m_CurrentPosition = 0;
}

std::size_t MemoryStream::GetAvailableBytes()
{
	return m_Storage.size() - m_CurrentPosition;
}

std::size_t MemoryStream::ReadBytes(std::span<std::byte> const& buffer)
{
	const auto readSize = std::min(static_cast<std::size_t>(buffer.size()), GetAvailableBytes());
	std::memcpy(buffer.data(), m_Storage.data() + m_CurrentPosition, readSize);
	m_CurrentPosition += readSize;

	return readSize;
}

std::size_t MemoryStream::Skip(std::size_t n)
{
	const auto skippedSize = std::min(n, GetAvailableBytes());
	m_CurrentPosition += skippedSize;
	return skippedSize;
}

std::size_t MemoryStream::GetPosition() const
{
	return m_CurrentPosition;
}

void MemoryStream::SeekFromBegin(std::size_t pos)
{
	assert(0 <= pos && pos <= m_Storage.size());
	m_CurrentPosition = pos;
}

void MemoryStream::Seek(SeekOrigin origin, std::ptrdiff_t diff)
{
	switch (origin)
	{
	default:
		assert(!"Invalid origin");
		[[fallthrough]];
	case SeekOrigin::Begin:
		if (diff < 0 || diff > m_Storage.size())
		{
			CAFE_THROW(IoException, CAFE_UTF8_SV("Out of range."));
		}
		m_CurrentPosition = diff;
		break;
	case SeekOrigin::Current:
		if (diff < 0 ? -diff > m_CurrentPosition : diff > GetAvailableBytes())
		{
			CAFE_THROW(IoException, CAFE_UTF8_SV("Out of range."));
		}

		m_CurrentPosition += diff;
		break;
	case SeekOrigin::End:
		if (diff > 0 || -diff > m_Storage.size())
		{
			CAFE_THROW(IoException, CAFE_UTF8_SV("Out of range."));
		}

		m_CurrentPosition = m_Storage.size() + diff;
		break;
	}
}

std::size_t MemoryStream::GetTotalSize()
{
	return m_Storage.size();
}

std::size_t MemoryStream::WriteBytes(std::span<const std::byte> const& buffer)
{
	const auto copySize = std::min(static_cast<std::size_t>(buffer.size()), GetAvailableBytes());
	std::memcpy(m_Storage.data() + m_CurrentPosition, buffer.data(), copySize);
	if (copySize != buffer.size())
	{
		m_Storage.insert(m_Storage.end(), buffer.begin() + copySize, buffer.end());
	}

	m_CurrentPosition += buffer.size();

	return buffer.size();
}

std::span<std::byte> MemoryStream::GetInternalStorage() noexcept
{
	return std::span(m_Storage.data(), m_Storage.size());
}

std::span<const std::byte> MemoryStream::GetInternalStorage() const noexcept
{
	return std::span(m_Storage.data(), m_Storage.size());
}

std::vector<std::byte> MemoryStream::ReleaseStorage() noexcept
{
	return std::move(m_Storage);
}

ExternalMemoryInputStream::ExternalMemoryInputStream(
    std::span<const std::byte> const& storage) noexcept
    : ExternalMemoryStreamCommonPart{ storage, false }
{
}

ExternalMemoryInputStream::ExternalMemoryInputStream(std::span<const std::byte> const& storage,
                                                     Detail::ErrorOnOutOfRangeTag) noexcept
    : ExternalMemoryStreamCommonPart{ storage, true }
{
}

ExternalMemoryInputStream::~ExternalMemoryInputStream()
{
}

std::size_t ExternalMemoryInputStream::GetAvailableBytes()
{
	return m_Storage.size() - GetPosition();
}

std::size_t ExternalMemoryInputStream::ReadBytes(std::span<std::byte> const& buffer)
{
	std::size_t readSize;
	const auto bufferSize = static_cast<std::size_t>(buffer.size());
	const auto availableSize = GetAvailableBytes();

	if (bufferSize > availableSize)
	{
		if (m_ErrorOnOutOfRange)
		{
			CAFE_THROW(IoException, CAFE_UTF8_SV("Out of range."));
		}

		readSize = availableSize;
	}
	else
	{
		readSize = bufferSize;
	}

	std::memcpy(buffer.data(), m_CurrentPosition, readSize);
	m_CurrentPosition += readSize;

	return readSize;
}

std::size_t ExternalMemoryInputStream::Skip(std::size_t n)
{
	const auto skippedSize = std::min(n, GetAvailableBytes());
	m_CurrentPosition += skippedSize;
	return skippedSize;
}

ExternalMemoryOutputStream::ExternalMemoryOutputStream(std::span<std::byte> const& storage) noexcept
    : ExternalMemoryStreamCommonPart{ storage, false }
{
}

ExternalMemoryOutputStream::ExternalMemoryOutputStream(std::span<std::byte> const& storage,
                                                       Detail::ErrorOnOutOfRangeTag) noexcept
    : ExternalMemoryStreamCommonPart{ storage, true }
{
}

ExternalMemoryOutputStream::~ExternalMemoryOutputStream()
{
}

std::size_t ExternalMemoryOutputStream::WriteBytes(std::span<const std::byte> const& buffer)
{
	std::size_t writtenSize;

	const auto bufferSize = static_cast<std::size_t>(buffer.size());
	const auto availableSize = m_Storage.size() - GetPosition();
	if (bufferSize > availableSize)
	{
		if (m_ErrorOnOutOfRange)
		{
			CAFE_THROW(IoException, CAFE_UTF8_SV("Out of range."));
		}

		writtenSize = availableSize;
	}
	else
	{
		writtenSize = bufferSize;
	}

	std::memcpy(m_CurrentPosition, buffer.data(), writtenSize);
	m_CurrentPosition += writtenSize;

	return writtenSize;
}
