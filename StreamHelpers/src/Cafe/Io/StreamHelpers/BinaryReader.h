#pragma once

#include <Cafe/Io/Streams/StreamBase.h>
#include <cstring>
#include <type_traits>

#ifdef __MSC_VER
#include <cstdlib> // 引入 _byteswap_ushort _byteswap_ulong _byteswap_uint64
#endif

namespace Cafe::Io
{
	static_assert(std::endian::native == std::endian::little ||
	              std::endian::native == std::endian::big);

	template <InputStreamConcept InputStreamType = InputStream>
	class BinaryReader
	{
	public:
		explicit BinaryReader(InputStreamType* stream,
		                      std::endian usingEndian = std::endian::native) noexcept
		    : m_Stream{ stream }, m_UsingEndian{ usingEndian }
		{
			assert(m_Stream);
			assert(m_UsingEndian == std::endian::little || m_UsingEndian == std::endian::big);
		}

		InputStreamType* GetStream() const noexcept
		{
			return m_Stream;
		}

		std::endian GetUsingEndian() const noexcept
		{
			return m_UsingEndian;
		}

		template <typename T>
		[[nodiscard]] std::enable_if_t<std::is_scalar_v<T>, bool> Read(T& value) const
		{
			if (m_UsingEndian == std::endian::native)
			{
				T readValue;
				if (m_Stream->ReadBytes(std::as_writable_bytes(
				        std::span(std::addressof(readValue), 1))) != sizeof(T))
				{
					return false;
				}

				value = readValue;
			}
			else
			{
				if constexpr (sizeof(T) == 1)
				{
					const auto byte = m_Stream->ReadByte();
					if (!byte)
					{
						return false;
					}
					reinterpret_cast<std::byte&>(value) = *byte;
				}
#if defined(__GNUC__) || defined(_MSC_VER)
				else if constexpr (sizeof(T) == 2)
				{
					std::uint16_t readValue;
					if (m_Stream->ReadBytes(std::as_writable_bytes(std::span(&readValue, 1))) != 2)
					{
						return false;
					}
#ifdef __GNUC__
					const auto swappedResult = __builtin_bswap16(readValue);
#else // _MSC_VER
					const auto swappedResult = _byteswap_ushort(readValue);
#endif
					std::memcpy(std::addressof(value), &swappedResult, 2);
				}
				else if constexpr (sizeof(T) == 4)
				{
					std::uint32_t readValue;
					if (m_Stream->ReadBytes(std::as_writable_bytes(std::span(&readValue, 1))) != 4)
					{
						return false;
					}
#ifdef __GNUC__
					const auto swappedResult = __builtin_bswap32(readValue);
#else // _MSC_VER
					const auto swappedResult = _byteswap_ulong(readValue);
#endif
					std::memcpy(std::addressof(value), &swappedResult, 4);
				}
				else if constexpr (sizeof(T) == 8)
				{
					std::uint64_t readValue;
					if (m_Stream->ReadBytes(std::as_writable_bytes(std::span(&readValue, 1))) != 8)
					{
						return false;
					}
#ifdef __GNUC__
					const auto swappedResult = __builtin_bswap64(readValue);
#else // _MSC_VER
					const auto swappedResult = _byteswap_uint64(readValue);
#endif
					std::memcpy(std::addressof(value), &swappedResult, 8);
				}
#endif
				else
				{
					std::byte buffer[sizeof(T)];
					if (m_Stream->ReadBytes(std::span(buffer)) != sizeof(T))
					{
						return false;
					}

					const auto ptr = std::addressof(value);
					auto read = buffer;
					for (auto p = reinterpret_cast<std::byte*>(ptr + 1);
					     p != reinterpret_cast<std::byte*>(ptr);)
					{
						*--p = *read++;
					}
				}
			}

			return true;
		}

		template <typename T>
		[[nodiscard]] std::enable_if_t<std::is_scalar_v<T>, std::optional<T>> Read() const
		{
			T value;
			if (Read(value))
			{
				return value;
			}

			return {};
		}

	private:
		InputStreamType* m_Stream;
		std::endian m_UsingEndian;
	};
} // namespace Cafe::Io
