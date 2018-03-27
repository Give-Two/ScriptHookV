#ifndef __PATTERN_H__
#define __PATTERN_H__

#include "..\ScriptHookV.h"

class Module
{
private:
	uintptr_t m_begin;
	uintptr_t m_end;
	DWORD m_size;
public:
	template<typename TReturn, typename TOffset>
	TReturn* getRVA(TOffset rva)
	{
		return (TReturn*)(m_begin + rva);
	}

	Module(void* module = GetModuleHandle(nullptr))
		: m_begin((uintptr_t)module), m_end(0)
	{
		PIMAGE_DOS_HEADER dosHeader = getRVA<IMAGE_DOS_HEADER>(0);
		PIMAGE_NT_HEADERS ntHeader = getRVA<IMAGE_NT_HEADERS>(dosHeader->e_lfanew);

		m_end = m_begin + ntHeader->OptionalHeader.SizeOfCode;
		m_size = ntHeader->OptionalHeader.SizeOfImage;
	}
	inline uintptr_t base() { return m_begin; }
	inline uintptr_t end() { return m_end; }
	inline DWORD size() { return m_size; }
};

namespace mem
{
	namespace traits
	{
		template <typename...>
		using void_t = void;

		template <typename Function, typename = void>
		struct is_invocable
			: std::false_type
		{ };

		template <typename Function>
		struct is_invocable<Function, void_t<typename std::result_of<Function>::type>>
			: std::true_type
		{ };

		template <typename Function>
		struct is_function_pointer
			: std::bool_constant<std::is_pointer<Function>::value && std::is_function<typename std::remove_pointer<typename std::decay<Function>::type>::type>::value>
		{ };
	}

	class handle
	{
	protected:
		void* _handle;

	public:
		handle()
			: _handle(nullptr)
		{ }

		handle(std::nullptr_t null)
			: _handle(null)
		{ }

		handle(void* p)
			: _handle(p)
		{ }

		template <typename T>
		handle(T* p)
			: _handle(const_cast<typename std::remove_cv<T>::type*>(p))
		{ }

		handle(const std::uintptr_t p)
			: _handle(reinterpret_cast<void*>(p))
		{ }

		handle(const handle& copy) = default;

		template <typename T>
		typename std::enable_if<std::is_pointer<T>::value, T>::type as() const
		{
			return reinterpret_cast<T>(this->_handle);
		}

		template <typename T>
		typename std::enable_if<std::is_lvalue_reference<T>::value, T>::type as() const
		{
			return *this->as<typename std::remove_reference<T>::type*>();
		}

		template <typename T>
		typename std::enable_if<std::is_array<T>::value, T&>::type as() const
		{
			return this->as<T&>();
		}

		template <typename T>
		typename std::enable_if<std::is_same<T, std::uintptr_t>::value, T>::type as() const
		{
			return reinterpret_cast<std::uintptr_t>(this->as<void*>());
		}

		template <typename T>
		typename std::enable_if<std::is_same<T, std::intptr_t>::value, T>::type as() const
		{
			return reinterpret_cast<std::intptr_t>(this->as<void*>());
		}

		bool operator==(const handle& rhs) const
		{
			return this->as<void*>() == rhs.as<void*>();
		}

		bool operator!=(const handle& rhs) const
		{
			return this->as<void*>() != rhs.as<void*>();
		}

		bool operator>(const handle& rhs) const
		{
			return this->as<void*>() > rhs.as<void*>();
		}

		bool operator<(const handle& rhs) const
		{
			return this->as<void*>() < rhs.as<void*>();
		}

		bool operator>=(const handle& rhs) const
		{
			return this->as<void*>() >= rhs.as<void*>();
		}

		bool operator<=(const handle& rhs) const
		{
			return this->as<void*>() <= rhs.as<void*>();
		}

		operator void*() const
		{
			return this->as<void*>();
		}

		template <typename T>
		handle save(typename std::enable_if<std::is_pointer<T>::value, T>::type& out) const
		{
			out = this->as<T>();

			return *this;
		}

		template <typename T>
		typename std::enable_if<std::is_integral<T>::value, handle>::type add(const T offset) const
		{
			return this->as<std::uintptr_t>() + offset;
		}

		template <typename T>
		handle rip(const T ipoffset) const
		{
			return this->add(ipoffset).add(this->as<int&>());
		}

		handle translate(const handle from, const handle to) const
		{
			return to.add(this->as<std::intptr_t>() - from.as<std::intptr_t>());
		}

#ifdef _MEMORYAPI_H_
		bool protect(const std::size_t size, const std::uint32_t newProtect, const std::uint32_t* oldProtect)
		{
			return VirtualProtect(this->as<void*>(), size, (DWORD)newProtect, (DWORD*)&oldProtect) == TRUE;
		}

		bool nop(const std::size_t size)
		{
			std::uint32_t oldProtect;

			if (this->protect(size, PAGE_EXECUTE_READWRITE, &oldProtect))
			{
				std::memset(this->as<void*>(), 0x90, size);

				this->protect(size, oldProtect, nullptr);

				return true;
			}

			return false;
		}

		inline bool set(const void * data, const std::size_t size)
		{
			std::uint32_t oldProtect;

			if (this->protect(size, PAGE_EXECUTE_READWRITE, &oldProtect))
			{
				std::memcpy(this->as<void*>(), data, size);

				this->protect(size, oldProtect, nullptr);

				return true;
			}

			return false;
		}

		template <typename T>
		inline void write(const T value)
		{
			static_assert(std::is_trivially_copyable<T>::value, "Type is not trivially copyable");

			this->as<T&>() = value;
		}

		template <typename... T>
		void write_args(const T... args)
		{
			std::uintptr_t off = 0;

			(void)std::initializer_list<int>
			{
				0, (this->add(off).write(args), off += sizeof(args))...
			};
		}

		template <typename T>
		bool write_vp(const T value)
		{
			std::uint32_t oldProtect;

			auto size = sizeof(value);

			if (this->protect(size, PAGE_EXECUTE_READWRITE, &oldProtect))
			{
				this->write(value);

				this->protect(size, oldProtect, nullptr);

				return true;
			}

			return false;
		}

		template <typename... T>
		bool write_args_vp(const T... args)
		{
			std::uint32_t oldProtect;

			auto size = std::valarray<std::size_t>({ sizeof(args)... }).sum();

			if (this->protect(size, PAGE_EXECUTE_READWRITE, &oldProtect))
			{
				this->write_args(args...);

				this->protect(size, oldProtect, nullptr);

				return true;
			}

			return false;
		}
#endif
	};

	template <typename T>
	struct _pattern
	{
	protected:
		using type = T;

		struct nibble
		{
			type expected;
			type mask;

			nibble()
				: mask()
				, expected()
			{ }

			bool matches(const type* value) const
			{
				return !((*value ^ expected) & mask);
			}
		};

		std::vector<nibble> nibbles;

		bool compare(const handle address) const
		{
			for (std::size_t i = 0; i < nibbles.size(); ++i)
			{
				if (!nibbles[i].matches(address.as<const type*>() + i))
				{
					return false;
				}
			}

			return true;
		}

	public:
		_pattern(const char* string)
		{
			static const std::uint8_t hex_char_table[256] =
			{
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
			};

			while (true)
			{
				nibble      current_nibble;
				std::size_t i = 0;

				while (const char c = *string++)
				{
					const std::uint8_t shift = (i & 1) ? 0 : 4;
					const std::uint8_t value = hex_char_table[static_cast<std::uint8_t>(c)];

					if (value != 0xFF)
					{
						reinterpret_cast<std::uint8_t*>(&current_nibble.mask)[i / 2] |= (0xF << shift);
						reinterpret_cast<std::uint8_t*>(&current_nibble.expected)[i / 2] |= (value << shift);
					}
					else if (c != '?' && shift == 4)
					{
						continue;
					}

					if (++i == (sizeof(type) * 2))
					{
						break;
					}
				}

				if (i > 0)
				{
					nibbles.push_back(current_nibble);
				}

				if (i < (sizeof(type) * 2))
				{
					break;
				}
			}
		}

		handle scan(const handle base, const handle end) const
		{
			end.add(-static_cast<std::intptr_t>(nibbles.size() * sizeof(type)));

			for (handle current = base; current < end; current = current.add(1))
			{
				if (compare(current))
				{
					return current;
				}
			}

			return nullptr;
		}

		std::vector<handle> scan_all(const handle base, const handle end) const
		{
			std::vector<handle> results;

			for (handle current = base; current = this->scan(current, end); current = current.add(1))
			{
				results.push_back(current);
			}

			return results;
		}
	};

#ifdef _INCLUDED_SMM
	bool _pattern<__m128i>::nibble::matches(const __m128i* value) const
	{
		return _mm_testz_si128(_mm_xor_si128(_mm_loadu_si128(value), expected), mask);
	}
#endif

#ifdef _INCLUDED_IMM
	bool _pattern<__m256i>::nibble::matches(const __m256i* value) const
	{
		return _mm256_testz_si256(_mm256_xor_si256(_mm256_loadu_si256(value), expected), mask);
	}
#endif

	using pattern = _pattern<std::uintptr_t>;

	class region
	{
	protected:
		handle      _base;
		std::size_t _size;

	public:
		region(handle base, std::size_t size)
			: _base(base)
			, _size(size)
		{ }

		handle base() const
		{
			return this->_base;
		}

		std::size_t size() const
		{
			return this->_size;
		}

		handle end() const
		{
			return this->add(this->size());
		}

		bool contains(const handle address) const
		{
			return (address >= this->base()) && (address < this->end());
		}

		template <typename T>
		handle add(const T offset) const
		{
			return this->base().add(offset);
		}

		handle distance(const handle pointer) const
		{
			return pointer.as<std::uintptr_t>() - this->base().as<std::uintptr_t>();
		}

		handle memcpy(const handle pointer)
		{
			return std::memcpy(base().as<void*>(), pointer.as<const void*>(), size());
		}

		handle memset(const std::uint8_t value)
		{
			return std::memset(base().as<void*>(), value, size());
		}

		handle scan(const pattern& pattern) const
		{
			return pattern.scan(this->base(), this->end());
		}

		std::vector<handle> scan_all(const pattern& pattern) const
		{
			return pattern.scan_all(this->base(), this->end());
		}

		std::string to_hex_string(bool padded = false)
		{
			static const char* hexTable = "0123456789ABCDEF";

			std::stringstream stream;

			for (std::size_t i = 0; i < size(); ++i)
			{
				if (i && padded)
				{
					stream << ' ';
				}

				stream << hexTable[(base().as<const std::uint8_t*>()[i] >> 4) & 0xF];
				stream << hexTable[(base().as<const std::uint8_t*>()[i] >> 0) & 0xF];
			}

			return stream.str();
		}
	};

#ifdef _WINNT_
	class module : public region
	{
	protected:
		module(handle base)
			: region(base, base.add(base.as<IMAGE_DOS_HEADER&>().e_lfanew).as<IMAGE_NT_HEADERS&>().OptionalHeader.SizeOfImage)
		{ }

	public:
		static module named(const char* name)
		{
			return GetModuleHandleA(name);
		}

		static module named(const wchar_t* name)
		{
			return GetModuleHandleW(name);
		}

		static module named(const std::nullptr_t)
		{
			return module::named(static_cast<char*>(nullptr));
		}

		static module main()
		{
			return module::named(nullptr);
		}
	};
#endif

	template <typename C>
	class safe_class
	{
	protected:
		C* const _handle;

	public:
		safe_class()
			: _handle(nullptr)
		{ }

		safe_class(C* const pClass)
			: _handle(pClass)
		{ }

		C* get() const
		{
			return _handle;
		}

		operator bool() const
		{
			return _handle != nullptr;
		}

		C* operator ->() const
		{
			return _handle;
		}

		C& operator *() const
		{
			return *_handle;
		}

		template <typename T>
		safe_class<T> operator >>(T* C::*pMember) const
		{
			return _handle ? (_handle->*pMember) : nullptr;
		}
	};

	template <typename Invoker>
	struct static_function
	{
	protected:
		static_assert(traits::is_function_pointer<Invoker>::value, "Invoker is not a function pointer");
		static_assert(traits::is_invocable<Invoker(void*)>::value, "Invoker is not invocable with a void*");

		Invoker invoker_;
		std::unique_ptr<void, void(*)(void*)> params_;

		template <typename Invocable>
		static_function(Invocable* p)
			: invoker_([](void* p) { return std::move(*static_cast<Invocable*>(p))(); })
			, params_(p, [](void* p) { delete static_cast<Invocable*>(p); })
		{ }

	public:
		static_function()
			: invoker_(nullptr)
			, params_(nullptr, nullptr)
		{ }

		template <typename Function, typename... Args>
		static_function(Function f, Args... args)
			: static_function(new auto([f, args...]{ return f(args...); }))
		{ }

		Invoker get_invoker() const
		{
			return invoker_;
		}

		void* get_params() const
		{
			return params_.get();
		}
	};

	inline std::uintptr_t get_offset(mem::handle address)
	{
		std::uintptr_t new_address = 0ull;

		auto module = mem::module::main();

		if (module.contains(address))
		{
			new_address = address.as<std::uintptr_t>() - module.base().as<std::uintptr_t>();
		}

		return new_address;
	};

	inline bool compare(const uint8_t* pData, const uint8_t* bMask, const char* sMask)
	{
		for (; *sMask; ++sMask, ++pData, ++bMask)
			if (*sMask == 'x' && *pData != *bMask)
				return false;

		return *sMask == NULL;
	}

	inline std::vector<DWORD64> get_string_addresses(std::string str)
	{
		std::string currentMask;
		static auto module = Module();
		const char* to_scan = str.c_str();
		for (uint8_t i = 0; i < strlen(to_scan); i++)
			currentMask += "x";
		const char* mask = currentMask.c_str();
		std::vector<DWORD64> foundAddrs;
		for (uint32_t i = 0; i < module.size(); ++i) {
			auto address = module.base() + i;
			if (compare((BYTE*)(address), (BYTE*)to_scan, mask)) {
				foundAddrs.push_back((address));
			}
		}

		return foundAddrs;
	}

	inline uintptr_t get_multilayer_pointer(uintptr_t base_address, std::vector<DWORD> offsets)
	{
		uintptr_t ptr = *(uintptr_t*)(base_address);
		if (!ptr) {
			return NULL;
		}
		auto level = offsets.size();

		for (auto i = 0; i < level; i++) {
			if (i == level - 1) {
				ptr += offsets[i];
				if (!ptr)
					return NULL;
				return ptr;
			}
			else {
				ptr = *(uint64_t*)(ptr + offsets[i]);
				if (!ptr)
					return NULL;
			}
		}

		return ptr;
	}

	template <typename T>
	T get_value(uintptr_t Addr, std::vector<DWORD> offsets)
	{
		if (Addr == NULL) return NULL;
		uintptr_t ML = get_multilayer_pointer(Addr, offsets);
		if (ML == NULL) return NULL;
		return *((T*)ML);
	}

	template <typename T>
	void set_value(uintptr_t Addr, std::vector<DWORD> offsets, T value) {
		uintptr_t ML = get_multilayer_pointer(Addr, offsets);
		if (ML == NULL) {
			return;
		}

		*reinterpret_cast<T*>(ML) = value;
	}
}

inline mem::handle operator""_Scan(const char* string, std::size_t)
{
	if (auto handle = mem::module::named(nullptr).scan(string))
	{
		return handle;
	}

	LOG_ERROR("Pattern Fail! - %s", string);

	return nullptr;
}

#endif // __PATTERN_H__