#pragma once

#include "StreamBase.h"
#include <vector>

namespace Cafe::Io
{
	class CAFE_PUBLIC MemoryStream : public SeekableStream<InputOutputStream>
	{
	public:
		MemoryStream();
		explicit MemoryStream(std::span<const std::byte> const& initialContent);
		explicit MemoryStream(std::vector<std::byte>&& initialStorage);
		~MemoryStream();

		void Close() override;

		std::size_t GetAvailableBytes() override;
		std::size_t ReadBytes(std::span<std::byte> const& buffer) override;
		std::size_t Skip(std::size_t n) override;

		std::size_t GetPosition() const override;
		void SeekFromBegin(std::size_t pos) override;
		void Seek(SeekOrigin origin, std::ptrdiff_t diff) override;
		std::size_t GetTotalSize() override;

		std::size_t WriteBytes(std::span<const std::byte> const& buffer) override;

		std::span<std::byte> GetInternalStorage() noexcept;
		std::span<const std::byte> GetInternalStorage() const noexcept;

		std::vector<std::byte> ReleaseStorage() noexcept;

	private:
		std::vector<std::byte> m_Storage;
		std::size_t m_CurrentPosition;
	};

	namespace Detail
	{
		template <typename BaseStream>
		class ExternalMemoryStreamCommonPart : public SeekableStream<BaseStream>
		{
			using ElementType =
			    std::conditional_t<std::is_same_v<BaseStream, InputStream>, const std::byte, std::byte>;

		protected:
			explicit ExternalMemoryStreamCommonPart(std::span<ElementType> const& storage,
			                                        bool errorOnOutOfRange) noexcept
			    : m_Storage{ storage }, m_CurrentPosition{ m_Storage.data() }, m_ErrorOnOutOfRange{
				      errorOnOutOfRange
			      }
			{
			}

		public:
			std::size_t GetPosition() const override
			{
				return m_CurrentPosition - m_Storage.data();
			}

			void SeekFromBegin(std::size_t pos) override
			{
				m_CurrentPosition = m_Storage.data() + pos;
			}

			void Seek(SeekOrigin origin, std::ptrdiff_t diff) override
			{
				switch (origin)
				{
				default:
					assert(!"Invalid origin.");
					[[fallthrough]];
				case SeekOrigin::Begin:
					assert(0 <= diff && diff <= m_Storage.size());
					m_CurrentPosition = m_Storage.data() + diff;
					break;
				case SeekOrigin::Current:
					assert(m_CurrentPosition - m_Storage.data() <= diff &&
					       diff <= m_Storage.data() + m_Storage.size() - m_CurrentPosition);
					m_CurrentPosition += diff;
					break;
				case SeekOrigin::End:
					assert(-m_Storage.size() <= diff && diff <= 0);
					m_CurrentPosition = m_Storage.data() + m_Storage.size() + diff;
					break;
				}
			}

			std::size_t GetTotalSize() override
			{
				return m_Storage.size();
			}

			std::span<ElementType> GetStorage() const noexcept
			{
				return m_Storage;
			}

		protected:
			std::span<ElementType> m_Storage;
			ElementType* m_CurrentPosition;
			bool m_ErrorOnOutOfRange;
		};

		struct ErrorOnOutOfRangeTag
		{
		};
	} // namespace Detail

	constexpr Detail::ErrorOnOutOfRangeTag ErrorOnOutOfRange{};

	class CAFE_PUBLIC ExternalMemoryInputStream
	    : public Detail::ExternalMemoryStreamCommonPart<InputStream>
	{
	public:
		explicit ExternalMemoryInputStream(std::span<const std::byte> const& storage) noexcept;
		ExternalMemoryInputStream(std::span<const std::byte> const& storage,
		                          Detail::ErrorOnOutOfRangeTag) noexcept;
		~ExternalMemoryInputStream();

		std::size_t GetAvailableBytes() override;
		std::size_t ReadBytes(std::span<std::byte> const& buffer) override;
		std::size_t Skip(std::size_t n) override;
	};

	class CAFE_PUBLIC ExternalMemoryOutputStream
	    : public Detail::ExternalMemoryStreamCommonPart<OutputStream>
	{
	public:
		explicit ExternalMemoryOutputStream(std::span<std::byte> const& storage) noexcept;
		ExternalMemoryOutputStream(std::span<std::byte> const& storage,
		                           Detail::ErrorOnOutOfRangeTag) noexcept;
		~ExternalMemoryOutputStream();

		std::size_t WriteBytes(std::span<const std::byte> const& buffer) override;
	};
} // namespace Cafe::Io
