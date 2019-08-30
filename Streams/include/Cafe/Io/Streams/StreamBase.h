#pragma once

#include <Cafe/ErrorHandling/CommonExceptions.h>
#include <Cafe/ErrorHandling/ErrorHandling.h>
#include <Cafe/Misc/Scope.h>
#include <cstddef>
#include <gsl/span>
#include <optional>

namespace Cafe::Io
{
	CAFE_DEFINE_GENERAL_EXCEPTION(IoException, ErrorHandling::SystemException);

	/// @brief  流
	struct CAFE_PUBLIC Stream
	{
		virtual ~Stream() = 0;

		/// @brief  关闭流，若流有刷新操作则进行刷新后再关闭
		/// @remark 流关闭后可能处于任何状态，除非另有说明否则不可再进行除析构以外的操作
		///         流关闭可能是空操作，此时不需重写本方法
		virtual void Close();
	};

	/// @brief  输入流
	struct CAFE_PUBLIC InputStream : virtual Stream
	{
		virtual ~InputStream();

		/// @brief  获得当前可用的字节数
		/// @remark 若对于当前类无意义或无法实现则结果为 0
		/// @see    InputStream::ReadAvailableBytes(gsl::span<std::byte> const&)
		virtual std::size_t GetAvailableBytes() = 0;

		/// @brief  从流中读取一个字节
		virtual std::optional<std::byte> ReadByte();

		/// @brief  从流中读取多个字节，读取的个数为 buffer 的大小
		/// @remark 若读取长度大于可用字节数，则阻塞到读取到足够字节数再返回
		/// @return 读取的长度，若为 0 则表示流已到结尾或 buffer 大小为 0，其他异常情况将会抛出
		virtual std::size_t ReadBytes(gsl::span<std::byte> const& buffer) = 0;

		/// @brief  从流中读取有效字节，读取的个数最多不超过 buffer 的大小
		/// @remark 本方法不会阻塞，若有效字节数不足够填充整个 buffer 则只会填充已有的部分
		///         当 InputStream::GetAvailableBytes() 无效时，本方法立即返回且不读取任何内容
		virtual std::size_t ReadAvailableBytes(gsl::span<std::byte> const& buffer);

		/// @brief  跳过 n 个字节
		/// @remark 可能阻塞并消费字节，若可能则推荐使用 SeekableStream::Seek
		virtual std::size_t Skip(std::size_t n);
	};

	/// @brief  输出流
	struct CAFE_PUBLIC OutputStream : virtual Stream
	{
		virtual ~OutputStream();

		/// @brief  写入一个字节到流内
		virtual bool WriteByte(std::byte value);

		/// @brief  写入多个字节到流内，buffer 内全部数据都将写出，并阻塞到写入完成为止
		/// @return 写入的字节数，仅供参考，对于特殊的流可能无意义或有其他特殊含义
		virtual std::size_t WriteBytes(gsl::span<const std::byte> const& buffer) = 0;

		/// @brief  刷新流，确保数据成功刷新，对于无缓存的流可能无操作
		virtual void Flush();
	};

	struct CAFE_PUBLIC InputOutputStream : virtual InputStream, virtual OutputStream
	{
		virtual ~InputOutputStream();
	};

	enum class SeekOrigin
	{
		Begin,
		Current,
		End
	};

	/// @brief  可寻位流
	/// @tparam BaseStream  基类流，为了减少菱形继承使用模板
	///                     仅能是 Stream InputStream OutputStream InputOutputStream
	template <typename BaseStream>
	struct SeekableStream;

	template <>
	struct CAFE_PUBLIC SeekableStream<Stream>
	{
		virtual ~SeekableStream();

		virtual std::size_t GetPosition() const = 0;

		/// @brief  从流开始处寻位
		/// @remark 与 Seek 名称不同并分开的原因是若名称相同，子类覆盖时需显式 using
		///         才能不隐藏父类同名实体，由于 begin 不可往前寻位，分离并提供接受 std::size_t
		///         的参数可以使寻位范围更大
		virtual void SeekFromBegin(std::size_t pos);

		virtual void Seek(SeekOrigin origin, std::ptrdiff_t diff) = 0;

		/// @brief  获取整个流的长度
		/// @remark 由于默认实现涉及 Seek 操作因此不能 const 修饰
		virtual std::size_t GetTotalSize();
	};

	template <>
	struct CAFE_PUBLIC SeekableStream<InputStream> : virtual InputStream,
	                                                 virtual SeekableStream<Stream>
	{
		virtual ~SeekableStream();

		std::size_t Skip(std::size_t n) override;
	};

	template <>
	struct CAFE_PUBLIC SeekableStream<OutputStream> : virtual OutputStream,
	                                                  virtual SeekableStream<Stream>
	{
		virtual ~SeekableStream();
	};

	template <>
	struct CAFE_PUBLIC SeekableStream<InputOutputStream>
	    : SeekableStream<InputStream>, SeekableStream<OutputStream>, InputOutputStream
	{
		virtual ~SeekableStream();
	};

#if CAFE_IO_STREAMS_USE_CONCEPTS
	template <typename T>
	concept InputStreamConcept = std::is_base_of_v<InputStream, T>;

	template <typename T>
	concept OutputStreamConcept = std::is_base_of_v<OutputStream, T>;

	template <typename T>
	concept InputOutputStreamConcept = InputStreamConcept<T>&& OutputStreamConcept<T>;

	template <typename T>
	concept SeekableStreamConcept = std::is_base_of_v<SeekableStream<Stream>, T>;
#endif
} // namespace Cafe::Io
