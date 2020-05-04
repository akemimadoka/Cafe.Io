#pragma once

#include <Cafe/Io/Streams/Config/StreamConfig.h>

#if CAFE_IO_STREAMS_INCLUDE_FILE_STREAM

#	include "StreamBase.h"
#	include "MemoryStream.h"
#	include <Cafe/Encoding/Strings.h>
#	include <Cafe/TextUtils/Misc.h>
#	include <filesystem>
#	include <cassert>

#	if defined(_WIN32)
#		include <Cafe/Encoding/CodePage/UTF-16.h>
#		include <fileapi.h>
#		include <handleapi.h>
#		include <memoryapi.h>
#		include <windef.h>
#		include <winbase.h>
#	elif defined(__linux__)
#		include <Cafe/Encoding/CodePage/UTF-8.h>
#		include <fcntl.h>
#		include <unistd.h>
#		if CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING
#			include <sys/mman.h>
#		endif
#	endif

namespace Cafe::Io
{
#	if defined(_WIN32)
	constexpr Encoding::CodePage::CodePageType PathNativeCodePage =
	    Encoding::CodePage::Utf16LittleEndian;

	inline Encoding::StringView<Encoding::CodePage::Utf16LittleEndian>
	PathToNativeString(std::filesystem::path const& path) noexcept
	{
		const auto& nativeString = path.native();
		return gsl::span(
		    reinterpret_cast<const Encoding::CodePage::CodePageTrait<
		        Encoding::CodePage::Utf16LittleEndian>::CharType*>(nativeString.data()),
		    nativeString.size());
	}
#	elif defined(__linux__)
	constexpr Encoding::CodePage::CodePageType PathNativeCodePage = Encoding::CodePage::Utf8;

	inline Encoding::StringView<Encoding::CodePage::Utf8>
	PathToNativeString(std::filesystem::path const& path) noexcept
	{
		const auto& nativeString = path.native();
		return gsl::span(
		    reinterpret_cast<
		        const Encoding::CodePage::CodePageTrait<Encoding::CodePage::Utf8>::CharType*>(
		        nativeString.data()),
		    nativeString.size());
	}
#	else
	constexpr Encoding::CodePage::CodePageType PathNativeCodePage = Encoding::CodePage::Utf8;

	inline Encoding::String<Encoding::CodePage::Utf8>
	PathToNativeString(std::filesystem::path const& path)
	{
		const auto string = path.u8string();
		return gsl::span(
		    reinterpret_cast<
		        const Encoding::CodePage::CodePageTrait<Encoding::CodePage::Utf8>::CharType*>(
		        string.data()),
		    string.size());
	}
#	endif

	CAFE_DEFINE_GENERAL_EXCEPTION(FileIoException, IoException);

	namespace Detail
	{
		template <typename BaseStream>
		struct FileStreamCommonPart : SeekableStream<BaseStream>
		{
		private:
			static constexpr bool IsInputStream = std::is_same_v<BaseStream, InputStream>;

		public:
			using NativeHandle =
#	if defined(_WIN32)
			    HANDLE
#	else
			    int
#	endif
			    ;

			// INVALID_HANDLE_VALUE 有强制转换，不能是 constexpr 的
			static inline const NativeHandle InvalidHandleValue =
#	if defined(_WIN32)
			    INVALID_HANDLE_VALUE
#	else
			    -1
#	endif
			    ;

			explicit FileStreamCommonPart(NativeHandle nativeHandle = InvalidHandleValue) noexcept
			    : m_FileHandle{ nativeHandle }, m_ShouldNotDestroy
			{
				false
			}
#	if CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING
#		if defined(_WIN32)
			, m_FileMapping{}, m_MappedFile
			{
			}
#		else
			, m_FileView{}, m_FileViewSize
			{
			}
#		endif
#	endif
			{
			}

			FileStreamCommonPart(FileStreamCommonPart const&) = delete;

			FileStreamCommonPart(FileStreamCommonPart&& other) noexcept
			    : m_FileHandle{ std::exchange(other.m_FileHandle, InvalidHandleValue) },
			      m_ShouldNotDestroy
			{
				other.m_ShouldNotDestroy
			}
#	if CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING
#		if defined(_WIN32)
			, m_FileMapping{ std::exchange(other.m_FileMapping, nullptr) }, m_MappedFile
			{
				std::exchange(other.m_MappedFile, nullptr)
			}
#		else
			, m_FileView{ std::exchange(other.m_FileView, nullptr) }, m_FileViewSize
			{
				std::exchange(other.m_FileViewSize, 0)
			}
#		endif
#	endif
			{
			}

			~FileStreamCommonPart()
			{
				// 析构函数中不会动态绑定，所以加上限定符
				FileStreamCommonPart::Close();
			}

			FileStreamCommonPart& operator=(FileStreamCommonPart const&) = delete;

			FileStreamCommonPart& operator=(FileStreamCommonPart&& other) noexcept
			{
				if (this != &other)
				{
					Close();
					m_FileHandle = std::exchange(other.m_FileHandle, InvalidHandleValue);
					m_ShouldNotDestroy = other.m_ShouldNotDestroy;
#	if CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING
#		if defined(_WIN32)
					m_FileMapping = std::exchange(other.m_FileMapping, nullptr);
					m_MappedFile = std::exchange(other.m_MappedFile, nullptr);
#		else
					m_FileView = std::exchange(other.m_FileView, nullptr);
					m_FileViewSize = std::exchange(other.m_FileViewSize, 0);
#		endif
#	endif
				}

				return *this;
			}

			void Close() override
			{
#	if CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING
				Unmap();
#	endif
				if (!m_ShouldNotDestroy && m_FileHandle != InvalidHandleValue)
				{
#	if defined(_WIN32)
					CloseHandle(m_FileHandle);
#	else
					close(m_FileHandle);
#	endif
					m_FileHandle = InvalidHandleValue;
				}
			}

			NativeHandle GetNativeHandle() const noexcept
			{
				return m_FileHandle;
			}

			std::size_t GetPosition() const override
			{
#	if defined(_WIN32)
				LARGE_INTEGER pos;
				if (!SetFilePointerEx(m_FileHandle, {}, &pos, static_cast<DWORD>(SeekOrigin::Current)))
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot fetch current position."));
				}
				return static_cast<std::size_t>(pos.QuadPart);
#	else
				const auto pos = lseek64(m_FileHandle, 0, static_cast<int>(SeekOrigin::Current));
				if (pos == off64_t(-1))
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot set position."));
				}
				return static_cast<std::size_t>(pos);
#	endif
			}

			void Seek(SeekOrigin origin, std::ptrdiff_t diff) override
			{
#	if defined(_WIN32)
				// 值刚好对应，不想为了几个宏引入太多依赖，因此不包含 Winbase.h
				const auto moveMethod = static_cast<DWORD>(origin);

				LARGE_INTEGER pos;
				pos.QuadPart = diff;
				if (!SetFilePointerEx(m_FileHandle, pos, nullptr, moveMethod))
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot set position."));
				}
#	else
				// SEEK_SET SEEK_CUR SEEK_END 刚好对应 SeekOrigin 的值
				const auto whence = static_cast<int>(origin);

				if (lseek64(m_FileHandle, static_cast<off64_t>(diff), whence) == off64_t(-1))
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot set position."));
				}
#	endif
			}

#	if defined(_WIN32)
			std::size_t GetTotalSize() override
			{
				LARGE_INTEGER size;
				if (!GetFileSizeEx(m_FileHandle, &size))
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("Cannot fetch file size."));
				}

				return static_cast<std::size_t>(size.QuadPart);
			}
#	endif

#	if CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING
			std::conditional_t<IsInputStream, ExternalMemoryInputStream, ExternalMemoryOutputStream>
			MapToMemory(std::size_t begin = 0, std::size_t size = 0, bool executable = false)
			{
				using MapStream = std::conditional_t<IsInputStream, ExternalMemoryInputStream,
				                                     ExternalMemoryOutputStream>;
#		if defined(_WIN32)
#			if defined(_WIN64)
				const DWORD sizeLowPart = size & 0xFFFFFFFF;
				const DWORD sizeHighPart = (size >> 32) & 0xFFFFFFFF;
#			else
				const DWORD sizeLowPart = size;
				const DWORD sizeHighPart = 0;
#			endif
				const auto fileMapping = CreateFileMappingA(
				    m_FileHandle, nullptr,
				    IsInputStream ? (executable ? PAGE_EXECUTE_READ : PAGE_READONLY)
				                  : (executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE),
				    sizeHighPart, sizeLowPart, nullptr);
				if (!fileMapping)
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("CreateFileMappingA failed."));
				}

				CAFE_SCOPE_FAIL
				{
					CloseHandle(fileMapping);
				};

#			if defined(_WIN64)
				const DWORD beginLowPart = begin & 0xFFFFFFFF;
				const DWORD beginHighPart = (begin >> 32) & 0xFFFFFFFF;
#			else
				const DWORD beginLowPart = begin;
				const DWORD beginHighPart = 0;
#			endif

				const auto mappedAddress = MapViewOfFile(fileMapping,
				                                         (IsInputStream ? FILE_MAP_READ : FILE_MAP_WRITE) |
				                                             (executable ? FILE_MAP_EXECUTE : 0),
				                                         beginHighPart, beginLowPart, size);

				if (!mappedAddress)
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("MapViewOfFile failed."));
				}

				m_FileMapping = fileMapping;
				m_MappedFile = mappedAddress;

				return MapStream{ gsl::span(
					  static_cast<std::conditional_t<IsInputStream, const std::byte, std::byte>*>(
					      mappedAddress),
					  size ? size : this->GetTotalSize()) };
#		else
				const auto mappedSize = size ? size : this->GetTotalSize();
				const auto mappedFile =
				    mmap64(nullptr, mappedSize,
				           PROT_READ | (IsInputStream ? 0 : PROT_WRITE) | (executable ? PROT_EXEC : 0),
				           MAP_SHARED, m_FileHandle, begin);
				if (mappedFile == MAP_FAILED)
				{
					CAFE_THROW(FileIoException, CAFE_UTF8_SV("mmap64 failed."));
				}

				m_FileView = mappedFile;
				m_FileViewSize = mappedSize;

				return MapStream{ gsl::span(
					  static_cast<std::conditional_t<IsInputStream, const std::byte, std::byte>*>(mappedFile),
					  mappedSize) };
#		endif
			}

			void Unmap() noexcept
			{
#		if defined(_WIN32)
				assert(!m_MappedFile == !m_FileMapping);
				if (m_MappedFile)
				{
					UnmapViewOfFile(m_MappedFile);
					CloseHandle(m_FileMapping);

					m_FileMapping = nullptr;
					m_MappedFile = nullptr;
				}
#		else
				assert(!m_FileView == !m_FileViewSize);
				if (m_FileView)
				{
					munmap(m_FileView, m_FileViewSize);

					m_FileView = nullptr;
					m_FileViewSize = 0;
				}
#		endif
			}

		protected:
			NativeHandle m_FileHandle;
			bool m_ShouldNotDestroy;

		private:
#		if defined(_WIN32)
			HANDLE m_FileMapping;
			void* m_MappedFile;
#		else
			void* m_FileView;
			std::size_t m_FileViewSize;
#		endif
#	endif
		};

		struct SpecifyNativeHandleTag
		{
			constexpr SpecifyNativeHandleTag() noexcept = default;
		};
	} // namespace Detail

	constexpr Detail::SpecifyNativeHandleTag SpecifyNativeHandle{};

	class CAFE_PUBLIC FileInputStream : public Detail::FileStreamCommonPart<InputStream>
	{
	public:
		explicit FileInputStream(std::filesystem::path const& path);
		explicit FileInputStream(Encoding::StringView<PathNativeCodePage> const& path);

		template <Encoding::CodePage::CodePageType CodePageValue>
		explicit FileInputStream(Encoding::StringView<CodePageValue> const& path)
		    : FileInputStream{ TextUtils::EncodeTo<PathNativeCodePage>(path).GetView() }
		{
		}

		/// @brief  直接以已获得的文件句柄构造
		/// @param  fileHandle      文件句柄
		/// @param  transferOwner   转移所有权，若为 true 则 Close() 会关闭此句柄
		explicit FileInputStream(Detail::SpecifyNativeHandleTag, NativeHandle fileHandle,
		                         bool transferOwner = true);

		FileInputStream(FileInputStream const&) = delete;
		FileInputStream(FileInputStream&&) = default;

		~FileInputStream();

		FileInputStream& operator=(FileInputStream const&) = delete;
		FileInputStream& operator=(FileInputStream&&) = default;

		std::size_t GetAvailableBytes() override;
		std::size_t ReadBytes(gsl::span<std::byte> const& buffer) override;

		std::size_t Skip(std::size_t n) override;

		static FileInputStream CreateStdInStream();
	};

	class CAFE_PUBLIC FileOutputStream : public Detail::FileStreamCommonPart<OutputStream>
	{
	public:
		enum class FileOpenMode
		{
			Truncate,
			Overwrite,
			Append
		};

		explicit FileOutputStream(std::filesystem::path const& path,
		                          FileOpenMode openMode = FileOpenMode::Truncate);
		explicit FileOutputStream(Encoding::StringView<PathNativeCodePage> const& path,
		                          FileOpenMode openMode = FileOpenMode::Truncate);

		template <Encoding::CodePage::CodePageType CodePageValue>
		explicit FileOutputStream(Encoding::StringView<CodePageValue> const& path,
		                          FileOpenMode openMode = FileOpenMode::Truncate)
		    : FileOutputStream{ TextUtils::EncodeTo<PathNativeCodePage>(path).GetView(), openMode }
		{
		}

		/// @brief  直接以已获得的文件句柄构造
		/// @param  fileHandle      文件句柄
		/// @param  transferOwner   转移所有权，若为 true 则 Close() 会关闭此句柄
		explicit FileOutputStream(Detail::SpecifyNativeHandleTag, NativeHandle fileHandle,
		                          bool transferOwner = true);

		FileOutputStream(FileOutputStream const&) = delete;
		FileOutputStream(FileOutputStream&&) = default;

		~FileOutputStream();

		FileOutputStream& operator=(FileOutputStream const&) = delete;
		FileOutputStream& operator=(FileOutputStream&&) = default;

		std::size_t WriteBytes(gsl::span<const std::byte> const& buffer) override;
		void Flush() override;

		static FileOutputStream CreateStdOutStream();
		static FileOutputStream CreateStdErrStream();
	};
} // namespace Cafe::Io

#endif
