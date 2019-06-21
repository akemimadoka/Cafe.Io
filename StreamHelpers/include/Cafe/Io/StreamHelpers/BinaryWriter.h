#pragma once

#include <Cafe/Io/Streams/StreamBase.h>
#include <cstring>
#include <type_traits>

#ifdef __MSC_VER
#	include <cstdlib> // 引入 _byteswap_ushort _byteswap_ulong _byteswap_uint64
#endif

namespace Cafe::Io
{
	static_assert(std::endian::native == std::endian::little ||
	              std::endian::native == std::endian::big);

	class BinaryWriter
	{
	public:
		explicit BinaryWriter(OutputStream* stream,
		                      std::endian usingEndian = std::endian::native) noexcept
		    : m_Stream{ stream }, m_UsingEndian{ usingEndian }
		{
			assert(m_Stream);
			assert(m_UsingEndian == std::endian::little || m_UsingEndian == std::endian::big);
		}

		template <typename T>
		std::enable_if_t<std::is_scalar_v<T>, bool> Write(T const& value) const
		{
			if (m_UsingEndian == std::endian::native)
			{
				return m_Stream->WriteBytes(gsl::as_bytes(gsl::make_span(std::addressof(value), 1))) ==
				       sizeof(T);
			}
			else
			{
				if constexpr (sizeof(T) == 1)
				{
					return m_Stream->WriteByte(reinterpret_cast<std::byte const&>(value));
				}
#if defined(__GNUC__) || defined(_MSC_VER)
				else if constexpr (sizeof(T) == 2)
				{
					std::uint16_t writeValue;
					std::memcpy(&writeValue, std::addressof(value), 2);
#	ifdef __GNUC__
					writeValue = __builtin_bswap16(writeValue);
#	else // _MSC_VER
					writeValue = _byteswap_ushort(writeValue);
#	endif
					return m_Stream->WriteBytes(gsl::as_bytes(gsl::make_span(&writeValue, 1))) == 2;
				}
				else if constexpr (sizeof(T) == 4)
				{
					std::uint32_t writeValue;
					std::memcpy(&writeValue, std::addressof(value), 4);
#	ifdef __GNUC__
					writeValue = __builtin_bswap32(writeValue);
#	else // _MSC_VER
					writeValue = _byteswap_ulong(writeValue);
#	endif
					return m_Stream->WriteBytes(gsl::as_bytes(gsl::make_span(&writeValue, 1))) == 4;
				}
				else if constexpr (sizeof(T) == 8)
				{
					std::uint64_t writeValue;
					std::memcpy(&writeValue, std::addressof(value), 8);
#	ifdef __GNUC__
					writeValue = __builtin_bswap64(writeValue);
#	else // _MSC_VER
					writeValue = _byteswap_uint64(writeValue);
#	endif
					return m_Stream->WriteBytes(gsl::as_bytes(gsl::make_span(&writeValue, 1))) == 8;
				}
#endif
				else
				{
					const auto pValue = &reinterpret_cast<std::byte const&>(value);
					std::byte buffer[sizeof(T)];
					auto pBuffer = buffer;
					for (auto p = pValue + sizeof(T); p != pValue;)
					{
						*pBuffer++ = *--p;
					}
					return m_Stream->WriteBytes(gsl::make_span(buffer)) == sizeof(T);
				}
			}
		}

	private:
		OutputStream* m_Stream;
		std::endian m_UsingEndian;
	};
} // namespace Cafe::Io
