#include <Cafe/Io/Streams/StlStream.h>

using namespace Cafe;
using namespace Io;

StlInputStream::StlInputStream(std::istream& stream) noexcept : m_Stream{ stream }
{
}

StlInputStream::~StlInputStream()
{
}

std::size_t StlInputStream::GetAvailableBytes()
{
	return m_Stream.rdbuf()->in_avail();
}

std::optional<std::byte> StlInputStream::ReadByte()
{
	std::byte value;
	if (ReadBytes(gsl::make_span(&value, 1)))
	{
		return value;
	}

	return {};
}

std::size_t StlInputStream::ReadBytes(gsl::span<std::byte> const& buffer)
{
	m_Stream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
	return m_Stream.gcount();
}

std::size_t StlInputStream::ReadAvailableBytes(gsl::span<std::byte> const& buffer)
{
	return m_Stream.readsome(reinterpret_cast<char*>(buffer.data()), buffer.size());
}

std::size_t StlInputStream::Skip(std::size_t n)
{
	const auto curPos = m_Stream.tellg();
	m_Stream.seekg(n, std::ios_base::cur);
	return m_Stream.tellg() - curPos;
}

std::size_t StlInputStream::GetPosition() const
{
	return m_Stream.tellg();
}

void StlInputStream::SeekFromBegin(std::size_t pos)
{
	m_Stream.seekg(pos);
}

void StlInputStream::Seek(SeekOrigin origin, std::ptrdiff_t diff)
{
	const auto seekDir = [&] {
		switch (origin)
		{
		default:
			assert(!"Invalid origin");
			[[fallthrough]];
		case SeekOrigin::Begin:
			return std::ios_base::beg;
		case SeekOrigin::Current:
			return std::ios_base::cur;
		case SeekOrigin::End:
			return std::ios_base::end;
		};
	}();

	m_Stream.seekg(diff, seekDir);
}

std::istream& StlInputStream::GetUnderlyingStream() const noexcept
{
	return m_Stream;
}

StlOutputStream::StlOutputStream(std::ostream& stream) noexcept : m_Stream{ stream }
{
}

StlOutputStream::~StlOutputStream()
{
}

bool StlOutputStream::WriteByte(std::byte value)
{
	return WriteBytes(gsl::make_span(&value, 1));
}

std::size_t StlOutputStream::WriteBytes(gsl::span<const std::byte> const& buffer)
{
	const auto curPos = m_Stream.tellp();
	m_Stream.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
	return m_Stream.tellp() - curPos;
}

void StlOutputStream::Flush()
{
	m_Stream.flush();
}

std::size_t StlOutputStream::GetPosition() const
{
	return m_Stream.tellp();
}

void StlOutputStream::SeekFromBegin(std::size_t pos)
{
	m_Stream.seekp(pos);
}

void StlOutputStream::Seek(SeekOrigin origin, std::ptrdiff_t diff)
{
	const auto seekDir = [&] {
		switch (origin)
		{
		default:
			assert(!"Invalid origin");
			[[fallthrough]];
		case SeekOrigin::Begin:
			return std::ios_base::beg;
		case SeekOrigin::Current:
			return std::ios_base::cur;
		case SeekOrigin::End:
			return std::ios_base::end;
		};
	}();

	m_Stream.seekp(diff, seekDir);
}

std::ostream& StlOutputStream::GetUnderlyingStream() const noexcept
{
	return m_Stream;
}

StlInputOutputStream::StlInputOutputStream(std::iostream& stream) noexcept : m_Stream{ stream }
{
}

StlInputOutputStream::~StlInputOutputStream()
{
}

std::size_t StlInputOutputStream::GetAvailableBytes()
{
	return StlInputStream{ m_Stream }.GetAvailableBytes();
}

std::optional<std::byte> StlInputOutputStream::ReadByte()
{
	return StlInputStream{ m_Stream }.ReadByte();
}

std::size_t StlInputOutputStream::ReadBytes(gsl::span<std::byte> const& buffer)
{
	return StlInputStream{ m_Stream }.ReadBytes(buffer);
}

std::size_t StlInputOutputStream::ReadAvailableBytes(gsl::span<std::byte> const& buffer)
{
	return StlInputStream{ m_Stream }.ReadAvailableBytes(buffer);
}

std::size_t StlInputOutputStream::Skip(std::size_t n)
{
	return StlInputStream{ m_Stream }.Skip(n);
}

bool StlInputOutputStream::WriteByte(std::byte value)
{
	return StlOutputStream{ m_Stream }.WriteByte(value);
}

std::size_t StlInputOutputStream::WriteBytes(gsl::span<const std::byte> const& buffer)
{
	return StlOutputStream{ m_Stream }.WriteBytes(buffer);
}

void StlInputOutputStream::Flush()
{
	StlOutputStream{ m_Stream }.Flush();
}

std::size_t StlInputOutputStream::GetPosition() const
{
	return StlInputStream{ m_Stream }.GetPosition();
}

void StlInputOutputStream::SeekFromBegin(std::size_t pos)
{
	StlInputStream{ m_Stream }.SeekFromBegin(pos);
}

void StlInputOutputStream::Seek(SeekOrigin origin, std::ptrdiff_t diff)
{
	StlInputStream{ m_Stream }.Seek(origin, diff);
}

std::iostream& StlInputOutputStream::GetUnderlyingStream() const noexcept
{
	return m_Stream;
}
