#pragma once

#include "StreamBase.h"
#include <iostream>

namespace Cafe::Io
{
	class CAFE_PUBLIC StlInputStream : public SeekableStream<InputStream>
	{
	public:
		explicit StlInputStream(std::istream& stream) noexcept;
		~StlInputStream();

		std::size_t GetAvailableBytes() override;
		std::optional<std::byte> ReadByte() override;
		std::size_t ReadBytes(gsl::span<std::byte> const& buffer) override;
		std::size_t ReadAvailableBytes(gsl::span<std::byte> const& buffer) override;
		std::size_t Skip(std::size_t n) override;

		std::size_t GetPosition() const override;
		void SeekFromBegin(std::size_t pos) override;
		void Seek(SeekOrigin origin, std::ptrdiff_t diff) override;

		std::istream& GetUnderlyingStream() const noexcept;

	private:
		std::istream& m_Stream;
	};

	class CAFE_PUBLIC StlOutputStream : public SeekableStream<OutputStream>
	{
	public:
		explicit StlOutputStream(std::ostream& stream) noexcept;
		~StlOutputStream();

		bool WriteByte(std::byte value) override;
		std::size_t WriteBytes(gsl::span<const std::byte> const& buffer) override;
		void Flush();

		std::size_t GetPosition() const override;
		void SeekFromBegin(std::size_t pos) override;
		void Seek(SeekOrigin origin, std::ptrdiff_t diff) override;

		std::ostream& GetUnderlyingStream() const noexcept;

	private:
		std::ostream& m_Stream;
	};

	class CAFE_PUBLIC StlInputOutputStream : public SeekableStream<InputOutputStream>
	{
	public:
		explicit StlInputOutputStream(std::iostream& stream) noexcept;
		~StlInputOutputStream();

        std::size_t GetAvailableBytes() override;
		std::optional<std::byte> ReadByte() override;
		std::size_t ReadBytes(gsl::span<std::byte> const& buffer) override;
		std::size_t ReadAvailableBytes(gsl::span<std::byte> const& buffer) override;
		std::size_t Skip(std::size_t n) override;

        bool WriteByte(std::byte value) override;
		std::size_t WriteBytes(gsl::span<const std::byte> const& buffer) override;
		void Flush();

		std::size_t GetPosition() const override;
		void SeekFromBegin(std::size_t pos) override;
		void Seek(SeekOrigin origin, std::ptrdiff_t diff) override;

		std::iostream& GetUnderlyingStream() const noexcept;

	private:
		std::iostream& m_Stream;
	};
} // namespace Cafe::Io
