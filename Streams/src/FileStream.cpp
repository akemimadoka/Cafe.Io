#include <Cafe/Io/Streams/FileStream.h>

using namespace Cafe;
using namespace Io;

FileInputStream::FileInputStream(std::filesystem::path const& path)
    : FileInputStream{ PathToNativeString(path) }
{
}

FileInputStream::FileInputStream(Encoding::StringView<PathNativeCodePage> const& path)
{
	Encoding::String<PathNativeCodePage> pathStr;
	const auto pathStrValue = path.IsNullTerminated() ? path.GetData() : (pathStr = path).GetData();

#if defined(_WIN32)
	m_FileHandle = CreateFileW(reinterpret_cast<LPCWSTR>(pathStrValue), GENERIC_READ, FILE_SHARE_READ,
	                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
#else
	m_FileHandle = open(pathStrValue, O_RDONLY);
#endif

	if (m_FileHandle == InvalidHandleValue)
	{
		CAFE_THROW(FileIoException, CAFE_UTF8_SV("Open file failed."));
	}
}

FileInputStream::~FileInputStream()
{
}

std::size_t FileInputStream::GetAvailableBytes()
{
	return GetTotalSize() - GetPosition();
}

std::optional<std::byte> FileInputStream::ReadByte()
{
	std::byte buffer[1];
	const auto readBytes = ReadBytes(gsl::make_span(buffer));
	if (readBytes)
	{
		return { buffer[0] };
	}

	return {};
}

std::size_t FileInputStream::ReadBytes(gsl::span<std::byte> const& buffer)
{
	if (buffer.empty())
	{
		return 0;
	}

	auto data = buffer.data();
	auto size = static_cast<std::size_t>(buffer.size());

#if defined(_WIN32)
	DWORD readSize;

	while (size)
	{
		if (!ReadFile(m_FileHandle, data,
		              static_cast<DWORD>(
		                  std::min(size, static_cast<std::size_t>(std::numeric_limits<DWORD>::max()))),
		              &readSize, NULL))
		{
			CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot read file."));
		}
		assert(size >= readSize);
		data += readSize;
		size -= readSize;
	}

	return buffer.size() - size;
#else
	const auto readSize = read(m_FileHandle, data, size);
	if (readSize == ssize_t(-1))
	{
		CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot read file."));
	}

	return static_cast<std::size_t>(readSize);
#endif
}

std::size_t FileInputStream::Skip(std::size_t n)
{
	n = std::min(n, GetAvailableBytes());

	if (n > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()))
	{
		Seek(SeekOrigin::Current, std::numeric_limits<std::ptrdiff_t>::max());
		Seek(SeekOrigin::Current,
		     static_cast<std::ptrdiff_t>(n - std::numeric_limits<std::ptrdiff_t>::max()));
	}
	else
	{
		Seek(SeekOrigin::Current, static_cast<std::ptrdiff_t>(n));
	}

	return n;
}

FileOutputStream::FileOutputStream(std::filesystem::path const& path)
    : FileOutputStream{ PathToNativeString(path) }
{
}

FileOutputStream::FileOutputStream(Encoding::StringView<PathNativeCodePage> const& path)
{
	Encoding::String<PathNativeCodePage> pathStr;
	const auto pathStrValue = path.IsNullTerminated() ? path.GetData() : (pathStr = path).GetData();

#if defined(_WIN32)
	m_FileHandle = CreateFileW(reinterpret_cast<LPCWSTR>(pathStrValue), GENERIC_READ, FILE_SHARE_READ,
	                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
#else
	m_FileHandle = open(pathStrValue, O_RDONLY);
#endif

	if (m_FileHandle == InvalidHandleValue)
	{
		CAFE_THROW(FileIoException, CAFE_UTF8_SV("Open file failed."));
	}
}

FileOutputStream::~FileOutputStream()
{
}

bool FileOutputStream::WriteByte(std::byte value)
{
	return WriteBytes(gsl::make_span(&value, 1));
}

std::size_t FileOutputStream::WriteBytes(gsl::span<const std::byte> const& buffer)
{
	if (buffer.empty())
	{
		return 0;
	}

	auto data = buffer.data();
	auto size = static_cast<std::size_t>(buffer.size());

#if defined(_WIN32)
	DWORD writtenSize;

	while (size)
	{
		if (!WriteFile(m_FileHandle, data,
		               static_cast<DWORD>(
		                   std::min(size, static_cast<std::size_t>(std::numeric_limits<DWORD>::max()))),
		               &writtenSize, nullptr))
		{
			CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot write file."));
		}
		assert(size >= writtenSize);
		size -= writtenSize;
		data += writtenSize;
	}

	return buffer.size() - size;
#else
	const auto writtenSize = write(m_FileHandle, data, size);
	if (writtenSize == ssize_t(-1))
	{
		CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot write file."));
	}

	return static_cast<std::size_t>(writtenSize);
#endif
}

void FileOutputStream::Flush()
{
#if defined(_WIN32)
	FlushFileBuffers(m_FileHandle);
#else
	fsync(m_FileHandle);
#endif
}
