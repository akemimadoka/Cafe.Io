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
	struct Stream
	{
		virtual ~Stream();

		/// @brief  关闭流，若流有刷新操作则进行刷新后再关闭
		/// @remark 流关闭后可能处于任何状态，除非另有说明否则不可再进行除析构以外的操作
		virtual void Close() = 0;
	};

	/// @brief  输入流
	struct InputStream : virtual Stream
	{
		virtual ~InputStream();

		/// @brief  获得当前可用的字节数
		virtual std::size_t GetAvailableBytes() = 0;

		/// @brief  从流中读取一个字节
		virtual std::optional<std::byte> ReadByte() = 0;

		/// @brief  从流中读取多个字节，读取的个数为 buffer 的大小
		/// @remark 若读取长度大于可用字节数，则阻塞到读取到足够字节数再返回
		/// @return 读取的长度，若为 0 则表示流已到结尾，其他异常情况将会抛出
		virtual std::size_t ReadBytes(gsl::span<std::byte> const& buffer);

		/// @brief  从流中读取有效字节，读取的个数最多不超过 buffer 的大小
		/// @remark 本方法不会阻塞，若有效字节数不足够填充整个 buffer 则只会填充已有的部分
		virtual std::size_t ReadAvailableBytes(gsl::span<std::byte> const& buffer);

		/// @brief  跳过 n 个字节
		/// @remark 可能阻塞并消费字节，若可能则推荐使用 SeekableStream::Seek
		virtual std::size_t Skip(std::size_t n);
	};

	/// @brief  输出流
	struct OutputStream : virtual Stream
	{
		virtual ~OutputStream();

		/// @brief  写入一个字节到流内
		virtual bool WriteByte(std::byte value) = 0;

		/// @brief  写入多个字节到流内，buffer 内全部数据都将写出，并阻塞到写入完成为止
		/// @return 写入的字节数，仅供参考，对于特殊的流可能无意义或有其他特殊含义
		virtual std::size_t WriteBytes(gsl::span<const std::byte> const& buffer) = 0;

		/// @brief  刷新流，确保数据成功刷新，对于无缓存的流无操作
		virtual void Flush();
	};

	struct InputOutputStream : InputStream, OutputStream
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
	/// @tparam 基类流，为了减少菱形继承使用模板
	template <typename BaseStream>
	struct SeekableStream : BaseStream
	{
		using BaseStream::BaseStream;

		virtual ~SeekableStream() = default;

		virtual std::size_t GetPosition() const = 0;

		/// @brief  从流开始处寻位
		/// @remark 与 Seek 名称不同并分开的原因是若名称相同，子类覆盖时需显式 using
		///         才能不隐藏父类同名实体，由于 begin 不可往前寻位，分离并提供接受 std::size_t
		///         的参数可以使寻位范围更大
		virtual void SeekFromBegin(std::size_t pos)
		{
			if (pos <= static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()))
			{
				Seek(SeekOrigin::Begin, static_cast<std::ptrdiff_t>(pos));
			}
			else
			{
				Seek(SeekOrigin::Begin, std::numeric_limits<std::ptrdiff_t>::max());
				Seek(SeekOrigin::Current,
				     static_cast<std::ptrdiff_t>(pos - std::numeric_limits<std::ptrdiff_t>::max()));
			}
		}

		virtual void Seek(SeekOrigin origin, std::ptrdiff_t diff) = 0;

		/// @brief  获取整个流的长度
		/// @remark 由于默认实现涉及 Seek 操作因此不能 const 修饰
		virtual std::size_t GetTotalSize()
		{
			const auto curPos = GetPosition();
			CAFE_SCOPE_EXIT
			{
				SeekFromBegin(curPos);
			};

			Seek(SeekOrigin::End, 0);
			return GetPosition();
		}
	};
} // namespace Cafe::Io
