#include <Cafe/Io/Streams/MemoryStream.h>
#include <cstring>

using namespace Cafe;
using namespace Io;

MemoryStream::MemoryStream() : m_CurrentPosition{}
{
}

MemoryStream::MemoryStream(gsl::span<const std::byte> const& initialContent)
    : m_Storage(initialContent.cbegin(), initialContent.cend()), m_CurrentPosition{}
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

std::size_t MemoryStream::ReadBytes(gsl::span<std::byte> const& buffer)
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
		assert(0 <= diff && diff <= m_Storage.size());
		m_CurrentPosition = diff;
		break;
	case SeekOrigin::Current:
		assert(-diff <= m_CurrentPosition && diff <= GetAvailableBytes());
		m_CurrentPosition += diff;
		break;
	case SeekOrigin::End:
		assert(-diff <= m_Storage.size() && diff <= 0);
		m_CurrentPosition = m_Storage.size() + diff;
		break;
	}
}

std::size_t MemoryStream::GetTotalSize()
{
	return m_Storage.size();
}

std::size_t MemoryStream::WriteBytes(gsl::span<const std::byte> const& buffer)
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

gsl::span<std::byte> MemoryStream::GetInternalStorage() noexcept
{
	return gsl::make_span(m_Storage);
}

gsl::span<const std::byte> MemoryStream::GetInternalStorage() const noexcept
{
	return gsl::make_span(m_Storage);
}

ExternalMemoryInputStream::ExternalMemoryInputStream(
    gsl::span<const std::byte> const& storage) noexcept
    : ExternalMemoryStreamCommonPart{ storage }
{
}

ExternalMemoryInputStream::~ExternalMemoryInputStream()
{
}

std::size_t ExternalMemoryInputStream::GetAvailableBytes()
{
	return m_Storage.size() - GetPosition();
}

std::size_t ExternalMemoryInputStream::ReadBytes(gsl::span<std::byte> const& buffer)
{
	const auto readSize = std::min(static_cast<std::size_t>(buffer.size()), GetAvailableBytes());
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

ExternalMemoryOutputStream::ExternalMemoryOutputStream(gsl::span<std::byte> const& storage) noexcept
    : ExternalMemoryStreamCommonPart{ storage }
{
}

ExternalMemoryOutputStream::~ExternalMemoryOutputStream()
{
}

std::size_t ExternalMemoryOutputStream::WriteBytes(gsl::span<const std::byte> const& buffer)
{
	const auto writtenSize =
	    std::min(static_cast<std::size_t>(buffer.size()), m_Storage.size() - GetPosition());
	std::memcpy(m_CurrentPosition, buffer.data(), writtenSize);
	m_CurrentPosition += writtenSize;

	return writtenSize;
}
