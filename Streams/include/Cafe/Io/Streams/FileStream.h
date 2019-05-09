#pragma once

#include <Cafe/Io/Streams/Config/StreamConfig.h>

#if CAFE_IO_STREAMS_INCLUDE_FILE_STREAM

#	include "StreamBase.h"
#	include <Cafe/Encoding/Strings.h>
#	include <Cafe/TextUtils/Misc.h>
#	include <filesystem>
#	include <cassert>

#	if defined(_WIN32)
#		include <Cafe/Encoding/CodePage/UTF-16.h>
#		include <fileapi.h>
#		include <handleapi.h>
#	elif defined(__linux__)
#		include <Cafe/Encoding/CodePage/UTF-8.h>
#		include <fcntl.h>
#		include <unistd.h>
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
		return gsl::make_span(
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
		return gsl::make_span(
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
		return gsl::make_span(
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
			    : m_FileHandle{ nativeHandle }
			{
			}

			FileStreamCommonPart(FileStreamCommonPart const&) = delete;

			FileStreamCommonPart(FileStreamCommonPart&& other) noexcept
			    : m_FileHandle{ std::exchange(other.m_FileHandle, InvalidHandleValue) }
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
				Close();
				m_FileHandle = std::exchange(other.m_FileHandle, InvalidHandleValue);

				return *this;
			}

			void Close() override
			{
				if (m_FileHandle != InvalidHandleValue)
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

		protected:
			NativeHandle m_FileHandle;
		};
	} // namespace Detail

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

		~FileInputStream();

		std::size_t GetAvailableBytes() override;
		std::optional<std::byte> ReadByte() override;
		std::size_t ReadBytes(gsl::span<std::byte> const& buffer) override;

		std::size_t Skip(std::size_t n) override;
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

		~FileOutputStream();

		bool WriteByte(std::byte value) override;
		std::size_t WriteBytes(gsl::span<const std::byte> const& buffer) override;
		void Flush() override;
	};
} // namespace Cafe::Io

#endif
