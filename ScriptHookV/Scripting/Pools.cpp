#include "Pools.h"
#include "NativeInvoker.h"

namespace rage
{
	class EntityPool
	{
	public:
		int _padding0[4];
		int  maxCount;          // num1
		int  itemSize;          // seems to be valid
		int  firstEmptySlot;    // these two increment and loop when hitting maxCount
		int  emptySlots;        // not sure exactly why
		int  itemCount;         // num2

		inline bool Full() const
		{
			return free() < 256;
		}

		int free() const 
		{
			return maxCount - (itemCount & 0x3FFFFFFF);
		}

	};

	class GenericPool
	{
	public:
		uint64_t poolStartAddress;
		uint8_t* byteArray;
		int  size;
		int  itemSize;

		inline bool isValid(int i)
		{
			assert(i >= 0);
			return mask(i) != 0;
		}

		inline uint64_t getAddress(int i)
		{
			assert(i >= 0);
			return mask(i) & (poolStartAddress + i * itemSize);
		}
	private:
		inline long long mask(int i)
		{
			assert(i >= 0);
			long long num1 = byteArray[i] & 0x80; // check for high bit.
			return ~((num1 | -num1) >> 63);
		}
	};

	class VehiclePool
	{
	public:
		uint64_t * listAddress;
		int  size;
		char _padding2[36];
		uint32_t* bitArray;
		char _padding3[40];
		int  itemCount;

		inline bool isValid(int i)
		{
			assert(i >= 0);
			return (bitArray[i >> 5] >> (i & 0x1F)) & 1;
		}

		inline uint64_t getAddress(int i)
		{
			return listAddress[i];
		}
	};

	struct EntityPoolTask
	{
		inline bool TypeHasFlag(uint16_t flag)
		{
			assert(!(_type & ~31));
			assert(!(flag & ~31));
			return (_type & flag) != 0;
		}

		EntityPoolTask(uint16_t type) : _type(type) {}

		void Run(std::vector<uintptr_t>& _pointers)
		{
			if (TypeHasFlag(PoolTypePed))
			{
				static GenericPool* &pedPool = "48 8B 05 ? ? ? ? 41 0F BF C8 0F BF 40 10"_Scan.add(3).rip(4).as<decltype(pedPool)>();

				for (int i = 0; i < pedPool->size; i++) 
				{
					if (uintptr_t address = pedPool->getAddress(i))
					{
						_pointers.push_back(address);
					}
				}
			}

			if (TypeHasFlag(PoolTypeVehicle))
			{
				static VehiclePool* &vehiclePool = *(VehiclePool **)(*(uintptr_t *)"48 8B 05 ? ? ? ? F3 0F 59 F6 48 8B 08"_Scan.add(3).rip(4).as<uintptr_t>());

				for (int i = 0; i < vehiclePool->size; i++)
				{
					if (vehiclePool->isValid(i))
					{
						if (uintptr_t address = vehiclePool->getAddress(i))
						{
							_pointers.push_back(address);
						}
					}
				}
			}

			if (TypeHasFlag(PoolTypeObject))
			{
				static GenericPool* &propPool = "48 8B 05 ? ? ? ? 8B 78 10 85 FF"_Scan.add(3).rip(4).as<decltype(propPool)>();

				for (int i = 0; i < propPool->size; i++)
				{
					if (uintptr_t address = propPool->getAddress(i))
					{
						_pointers.push_back(address);
					}
				}
			}

			if (TypeHasFlag(PoolTypePickup))
			{
				static GenericPool* &pickupPool = "4C 8B 05 ? ? ? ? 40 8A F2 8B E9"_Scan.add(3).rip(4).as<decltype(pickupPool)>();
				
				for (int i = 0; i < pickupPool->size; i++)
				{
					if (uintptr_t address = pickupPool->getAddress(i))
					{
						_pointers.push_back(address);
					}
				}
			}

			if (TypeHasFlag(PoolTypeCamera))
			{
				static GenericPool* &cameraPool = "48 8B C8 EB 02 33 C9 48 85 C9 74 26"_Scan.add(-9).rip(4).as<decltype(cameraPool)>();

				for (int i = 0; i < cameraPool->size; i++)
				{
					if (uintptr_t address = cameraPool->getAddress(i))
					{
						_pointers.push_back(address);
					}
				}
			}
		}
	private:
		uint16_t _type;
	};

	std::string GetEntityPoolStats()
	{
		static EntityPool* &entityPool = "4C 8B 0D ? ? ? ? 44 8B C1 49 8B 41 08"_Scan.add(3).rip(4).as<decltype(entityPool)>();

		return FMT("maxCount: %i  itemSize: %i  firstEmptySlot: %i  emptySlots: %i  free: %i",
			entityPool->maxCount,       // num1
			entityPool->itemSize,       // 16 bytes
			entityPool->firstEmptySlot, // not sure if these are valid for EntityPools -- sfink
			entityPool->emptySlots,     // not sure if these are valid for EntityPools -- sfink
			entityPool->maxCount - (entityPool->itemCount & 0x3FFFFFFF)
		);
	}

	void GetEntityPointers(EntityPoolType type, std::vector<uintptr_t>& result)
	{
        EntityPoolTask(type).Run(result);
    }  

	std::vector<Entity> GetAllWorld(EntityPoolType type, int max)
	{
		int count = 0;
		std::vector<Entity> entities;
		std::vector<uintptr_t> pointers;
		GetEntityPointers(type, pointers);

		for (const auto& cEntity : pointers)
		{
			if (count == max) break;
			auto entity = rage::AddressToEntity(cEntity);
			if (entity)
			{
				entities.push_back(entity);
				count++;
			}
		}

		return entities;
	}	

	int GetAllWorld(EntityPoolType type, int max, int *arr)
	{
		auto entities = GetAllWorld(type, max);

		for (int i = 0; i < entities.size(); ++i)
		{
			arr[i] = entities[i];
		}

		return (int) entities.size();	
	}
}