#pragma once

#include "StreamBase.h"
#include <memory>

namespace Cafe::Io
{
	/// @brief  缓存输入流
	/// @remark 用于频繁小长度的读取时降低 IO 压力
	///         本类不会取得包装流的所有权，在本类管理期间不应在外部操作包装流，否则可能导致错误
	class CAFE_PUBLIC BufferedInputStream : SeekableStream<InputStream>
	{
	public:
		static constexpr std::size_t DefaultBufferSize = 1024;

		explicit BufferedInputStream(InputStream* stream,
		                             std::size_t maxBufferSize = DefaultBufferSize);

		BufferedInputStream(BufferedInputStream const&) = delete;
		BufferedInputStream(BufferedInputStream&& other) noexcept;

		~BufferedInputStream();

		BufferedInputStream& operator=(BufferedInputStream const&) = delete;
		BufferedInputStream& operator=(BufferedInputStream&& other) noexcept;

		/// @remark 关闭时若包装流是可寻位的将会设为用户当前读取的位置，并释放缓存
		///         之后流处于无效状态，不可进行除析构以外的任何操作
		void Close() override;

		std::size_t GetAvailableBytes() override;
		std::optional<std::byte> ReadByte() override;
		std::size_t ReadBytes(gsl::span<std::byte> const& buffer) override;
		std::size_t Skip(std::size_t n) override;

		std::size_t GetPosition() const override;
		void SeekFromBegin(std::size_t pos) override;
		void Seek(SeekOrigin origin, std::ptrdiff_t diff) override;
		std::size_t GetTotalSize() override;

		std::size_t GetMaxBufferSize() const noexcept;
		std::size_t GetBufferSize() const noexcept;
		std::size_t GetAvailableBufferSize() const noexcept;

		InputStream* GetUnderlyingStream() const noexcept;

		std::optional<std::byte> PeekByte();
		/// @brief  不消费地读取一段数据
		/// @remark 若 buffer 大小大于实际缓存大小，仅会读取实际缓存
		std::size_t PeekBytes(gsl::span<std::byte> const& buffer);

	private:
		InputStream* m_UnderlyingStream;
		std::size_t m_LastReadBufferPosition;
		std::unique_ptr<std::byte[]> m_Buffer;
		std::size_t m_MaxBufferSize;
		std::size_t m_ReadSize;
		std::size_t m_CurrentPosition;

		void FlushBuffer(bool keep = true, std::size_t needSize = std::size_t(-1));
		void FillBuffer(bool keep = true, std::size_t needSize = std::size_t(-1));
	};

	/// @brief  缓存输出流
	/// @remark 用于频繁小长度的写入时降低 IO 压力
	///         本类不会取得包装流的所有权，在本类管理期间不应在外部操作包装流，否则可能导致错误
	class CAFE_PUBLIC BufferedOutputStream : OutputStream
	{
	public:
		static constexpr std::size_t DefaultBufferSize = 1024;

		explicit BufferedOutputStream(OutputStream* stream, std::size_t bufferSize = DefaultBufferSize);
		BufferedOutputStream(BufferedOutputStream const&) = delete;
		BufferedOutputStream(BufferedOutputStream&& other) noexcept;

		~BufferedOutputStream();

		BufferedOutputStream& operator=(BufferedOutputStream const&) = delete;
		BufferedOutputStream& operator=(BufferedOutputStream&& other) noexcept;

		/// @remark 关闭时将会向包装流写出所有缓存内容，并释放缓存
		///         之后流处于无效状态，不可进行除析构以外的任何操作
		void Close() override;

		bool WriteByte(std::byte value) override;
		std::size_t WriteBytes(gsl::span<const std::byte> const& buffer) override;
		void Flush() override;

	private:
		OutputStream* m_UnderlyingStream;
		std::unique_ptr<std::byte[]> m_Buffer;
		std::size_t m_BufferSize;
		std::size_t m_CurrentPosition;
	};
} // namespace Cafe::Io
