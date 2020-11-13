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
	const auto pathStrValue = reinterpret_cast<const char*>(
	    path.IsNullTerminated() ? path.GetData() : (pathStr = path).GetData());

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

FileInputStream::FileInputStream(Detail::SpecifyNativeHandleTag, NativeHandle fileHandle,
                                 bool transferOwner)
    : FileStreamCommonPart{ fileHandle }
{
	m_ShouldNotDestroy = !transferOwner;
	if (m_FileHandle == InvalidHandleValue || !fileHandle)
	{
		CAFE_THROW(FileIoException, CAFE_UTF8_SV("Invalid fileHandle."));
	}
}

FileInputStream::~FileInputStream()
{
}

std::size_t FileInputStream::GetAvailableBytes()
{
	return GetTotalSize() - GetPosition();
}

std::size_t FileInputStream::ReadBytes(std::span<std::byte> const& buffer)
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

FileInputStream FileInputStream::CreateStdInStream()
{
#ifdef _WIN32
	return FileInputStream{ SpecifyNativeHandle, GetStdHandle(STD_INPUT_HANDLE), false };
#else
	return FileInputStream{ SpecifyNativeHandle, STDIN_FILENO, false };
#endif
}

FileOutputStream::FileOutputStream(std::filesystem::path const& path, FileOpenMode openMode)
    : FileOutputStream{ PathToNativeString(path), openMode }
{
}

FileOutputStream::FileOutputStream(Encoding::StringView<PathNativeCodePage> const& path,
                                   FileOpenMode openMode)
{
	Encoding::String<PathNativeCodePage> pathStr;
	const auto pathStrValue = reinterpret_cast<const char*>(
	    path.IsNullTerminated() ? path.GetData() : (pathStr = path).GetData());

#if defined(_WIN32)
	m_FileHandle = CreateFileW(reinterpret_cast<LPCWSTR>(pathStrValue), GENERIC_READ | GENERIC_WRITE,
	                           0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	switch (openMode)
	{
	default:
		assert(!"Invalid openMode.");
		[[fallthrough]];
	case FileOpenMode::Truncate:
		// 分离 Truncate 实现是因为 TRUNCATE_EXISTING 在文件不存在时报错
		SetEndOfFile(m_FileHandle);
		break;
	case FileOpenMode::Append:
		FileOutputStream::Seek(SeekOrigin::End, 0);
		break;
	case FileOpenMode::Overwrite:
		break;
	}
#else

	auto openModeValue = O_CREAT | O_RDWR;
	switch (openMode)
	{
	default:
		assert(!"Invalid openMode.");
		[[fallthrough]];
	case FileOpenMode::Truncate:
		openModeValue |= O_TRUNC;
		break;
	case FileOpenMode::Append:
		openModeValue |= O_APPEND;
		break;
	case FileOpenMode::Overwrite:
		break;
	}

	m_FileHandle = open(pathStrValue, openModeValue, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

	if (m_FileHandle == InvalidHandleValue)
	{
		CAFE_THROW(FileIoException, CAFE_UTF8_SV("Open file failed."));
	}
}

FileOutputStream::FileOutputStream(Detail::SpecifyNativeHandleTag, NativeHandle fileHandle,
                                   bool transferOwner)
    : FileStreamCommonPart{ fileHandle }
{
	m_ShouldNotDestroy = !transferOwner;
	if (m_FileHandle == InvalidHandleValue || !fileHandle)
	{
		CAFE_THROW(FileIoException, CAFE_UTF8_SV("Invalid fileHandle."));
	}
}

FileOutputStream::~FileOutputStream()
{
}

std::size_t FileOutputStream::WriteBytes(std::span<const std::byte> const& buffer)
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

FileOutputStream FileOutputStream::CreateStdOutStream()
{
#ifdef _WIN32
	return FileOutputStream{ SpecifyNativeHandle, GetStdHandle(STD_OUTPUT_HANDLE), false };
#else
	return FileOutputStream{ SpecifyNativeHandle, STDOUT_FILENO, false };
#endif
}

FileOutputStream FileOutputStream::CreateStdErrStream()
{
#ifdef _WIN32
	return FileOutputStream{ SpecifyNativeHandle, GetStdHandle(STD_ERROR_HANDLE), false };
#else
	return FileOutputStream{ SpecifyNativeHandle, STDERR_FILENO, false };
#endif
}
