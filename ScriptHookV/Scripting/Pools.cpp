#include "Pools.h"
#include "NativeInvoker.h"

#define POOL_ASSERT(X) \
    ASSERT(X)

#define CHECK_ENTITY_POOL                                             \
    do {                                                              \
        if (entityPool->Full()) {                                     \
            LOG_DEBUG("Entity pool is full: %i", entityPool->free()); \
            return;                                                   \
        }                                                             \
    } while(0)

#define RECAST64(X) \
		((uint64_t)(X))

#define RETURN_UNLESS(XCONDITION, ...) \
	if (!(XCONDITION)) return __VA_ARGS__


namespace internal {

    UINT64 *_entityPoolAddress, *_vehiclePoolAddress, *_pedPoolAddress, *_objectPoolAddress, *_cameraPoolAddress, *_pickupObjectPoolAddress;
  
	static int64_t(*EntityModelHasher1)(int64_t) = "0F B7 05 ? ? ? ? 44 8B 49 18 45 33 C0 66 85 C0 74 2B 0F B7 C8 33 D2"_Scan.as<decltype(EntityModelHasher1)>();
	static int64_t(*EntityModelHasher2)(int64_t) = "40 53 48 83 ec 20 48 8b d9 e8 ? ? ? ? 8b 13 66 89 44 24 30 8b 44 24 30 8b ca 33 c8 81 e1 00 00 ff 0f 33 c1 48 8d 4c 24 30 0f ba f0 1d 33 d0 81 e2 00 00 00 10 33 c2 25 ff ff ff 3f 89 44 24 30 e8 ? ? ? ? 45"_Scan.as<decltype(EntityModelHasher2)>();

    void InitPools()
    {
		if (auto pattern = "4C 8B 0D ? ? ? ? 44 8B C1 49 8B 41 08"_Scan) {
			_entityPoolAddress = pattern.add(3).rip(4).as<decltype(_entityPoolAddress)>();
			LOG_ADDRESS("EntityPoolAddr", _entityPoolAddress);
		}	else { _entityPoolAddress = NULL; LOG_ERROR("Failed to find '_entityPoolAddress'"); }

		if (auto pattern = "48 8B 05 ? ? ? ? F3 0F 59 F6 48 8B 08"_Scan)
		{
			_vehiclePoolAddress = pattern.add(3).rip(4).as<decltype(_vehiclePoolAddress)>();
			LOG_ADDRESS("VehiclePoolAddr", _vehiclePoolAddress);
		}	else { _vehiclePoolAddress = NULL; LOG_ERROR("Failed to find '_vehiclePoolAddress'"); }

		if (auto pattern = "48 8B 05 ? ? ? ? 41 0F BF C8 0F BF 40 10"_Scan)
		{
			_pedPoolAddress = pattern.add(3).rip(4).as<decltype(_pedPoolAddress)>();
			LOG_ADDRESS("PedPoolAddr\t", _pedPoolAddress);
		}	else { _pedPoolAddress = NULL; LOG_ERROR("Failed to find '_pedPoolAddress'"); }

		if (auto pattern = "48 8B 05 ? ? ? ? 8B 78 10 85 FF"_Scan)
		{
			_objectPoolAddress = pattern.add(3).rip(4).as<decltype(_objectPoolAddress)>();
			LOG_ADDRESS("ObjectPoolAddr", _objectPoolAddress);
		}	else { _objectPoolAddress = NULL; LOG_ERROR("Failed to find '_objectPoolAddress'"); }

		if (auto pattern = "4C 8B 05 ? ? ? ? 40 8A F2 8B E9"_Scan)
		{
			_pickupObjectPoolAddress = pattern.add(3).rip(4).as<decltype(_pickupObjectPoolAddress)>();
			LOG_ADDRESS("PickupPoolAddr", _pickupObjectPoolAddress);
		}	else { _pickupObjectPoolAddress = NULL; LOG_ERROR("Failed to find '_pickupObjectPoolAddress'"); }

		if (auto pattern = "48 8B C8 EB 02 33 C9 48 85 C9 74 26"_Scan)
		{
			_cameraPoolAddress = pattern.add(-9).rip(4).as<decltype(_cameraPoolAddress)>();
			LOG_ADDRESS("CameraPoolAddr", _cameraPoolAddress);
		}	else { _cameraPoolAddress = NULL; LOG_ERROR("Failed to find '_cameraPoolAddress'"); }
    }

    class EntityPool
    {
    public:
        int _padding0[4];
        int  maxCount;          // num1
        int  itemSize;          // seems to be valid
        int  firstEmptySlot;    //   these two increment and loop when hitting maxCount
        int  emptySlots;        //   not sure exactly why
        int  itemCount;         // num2

        inline bool Full() const
        {
            return free() < 256;
            //return maxCount - (itemCount & 0x3FFFFFFF) <= 256;
        }

        int free() const {
            return maxCount - (itemCount & 0x3FFFFFFF);
        }

        static_assert(5 - (1 & 3) == 4, "Bracket ordering is incorrect");
    };
    struct __declspec(align(8)) WGAMemoryPool
    {
        UINT64 *listAddress;
        PBYTE byteArray;
        int maxCount;
        int itemSize;
        int firstEmptySlot;
        int emptySlots;
        int itemCount;
    };

    class VehiclePool
    {
    public:
        UINT64 *listAddress;
        int  size;
        char _padding2[36];
        UINT32* bitArray;
        char _padding3[40];
        int  itemCount;

        inline bool isValid(int i)
        {
            POOL_ASSERT(i >= 0);
            return (bitArray[i >> 5] >> (i & 0x1F)) & 1;
        }

        inline UINT64 getAddress(int i)
        {
            return listAddress[i];
        }
    };
    class GenericPool {
    public:
        UINT64 poolStartAddress;
        BYTE* byteArray;
        int  size;
        int  itemSize;

        inline bool isValid(int i)
        {
            POOL_ASSERT(i >= 0);
            return mask(i) != 0;
        }

        inline UINT64 getAddress(int i)
        {
            POOL_ASSERT(i >= 0);
            return mask(i) & (poolStartAddress + i * itemSize);
        }
    private:
        inline long long mask(int i)
        {
            POOL_ASSERT(i >= 0);
            long long num1 = byteArray[i] & 0x80; // check for high bit.
            return ~((num1 | -num1) >> 63);
        }
    };
    struct EntityPoolTask
    {
        enum Type : UINT16
        {
            Vehicle = 1,
            Ped = 2,
            Object = 4,
            Pickup = 8,
            Camera = 16
        };
        inline bool TypeHasFlag(UINT16 flag)
        {
            POOL_ASSERT(!(_type & ~31));
            POOL_ASSERT(!(flag & ~31));
            return (_type & flag) != 0;
        }

        EntityPoolTask(UINT16 type) : _type(type), _posCheck(false), _modelCheck(false)
        {
        }

        inline bool CheckEntity(uintptr_t address)
        {
			if (!mem::module::main().contains(address)) return false;
            if (_modelCheck)
            {
                UINT32 v0 = *reinterpret_cast<UINT32 *>(EntityModelHasher1(*reinterpret_cast<UINT64 *>(address + 32)));
                UINT32 v1 = v0 & 0xFFFF;
                UINT32 v2 = ((v1 ^ v0) & 0x0FFF0000 ^ v1) & 0xDFFFFFFF;
                UINT32 v3 = ((v2 ^ v0) & 0x10000000 ^ v2) & 0x3FFFFFFF;
                const UINT64 v5 = EntityModelHasher2(reinterpret_cast<uintptr_t>(&v3));

                if (!v5)
                {
                    return false;
                }
                for (int hash : _modelHashes)
                {
                    if (*reinterpret_cast<int *>(v5 + 24) == hash)
                    {
                        return true;
                    }
                }
                return false;
            }

            return true;
        }

        void Run(std::vector<uintptr_t>& _pointers)
        {
            if (TypeHasFlag(Type::Vehicle)) {
                if (*_vehiclePoolAddress) {
                    VehiclePool* vehiclePool = *reinterpret_cast<VehiclePool**>(*_vehiclePoolAddress);

                    for (int i = 0; i < vehiclePool->size; i++) {
                        if (vehiclePool->isValid(i)) {
                            uintptr_t address = vehiclePool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _pointers.push_back(address);
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Ped)) {
                if (*_pedPoolAddress) {
                    GenericPool* pedPool = reinterpret_cast<GenericPool*>(*_pedPoolAddress);

                    for (int i = 0; i < pedPool->size; i++) {
                        if (pedPool->isValid(i)) {
                            uintptr_t address = pedPool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _pointers.push_back(address);
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Object)) {
                if (*_objectPoolAddress) {
                    GenericPool* propPool = reinterpret_cast<GenericPool*>(*_objectPoolAddress);

                    for (int i = 0; i < propPool->size; i++) {
                        if (propPool->isValid(i)) {
                            uintptr_t address = propPool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _pointers.push_back(address);
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Camera)) {
                if (*_cameraPoolAddress) {
                    GenericPool* propPool = reinterpret_cast<GenericPool*>(*_cameraPoolAddress);

                    for (int i = 0; i < propPool->size; i++) {
                        if (propPool->isValid(i)) {
                            uintptr_t address = propPool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _pointers.push_back(address);
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Pickup)) {
                if (*_pickupObjectPoolAddress) {
                    GenericPool* pickupPool = reinterpret_cast<GenericPool*>(*_pickupObjectPoolAddress);

                    for (int i = 0; i < pickupPool->size; i++) {
                        if (pickupPool->isValid(i)) {
                            uintptr_t address = pickupPool->getAddress(i);
                            if (address) {
                                _pointers.push_back(address);
                            }
                        }
                    }
                }
            }
        }

        void Run(std::vector<int>& _handles)
        {
            if (!*_entityPoolAddress) {
                return;
            }
            EntityPool* entityPool = reinterpret_cast<EntityPool*>(*_entityPoolAddress);

            if (TypeHasFlag(Type::Vehicle)) {
                if (*_vehiclePoolAddress) {
                    VehiclePool* vehiclePool = *reinterpret_cast<VehiclePool**>(*_vehiclePoolAddress);

                    for (int i = 0; i < vehiclePool->size; i++) {
                        CHECK_ENTITY_POOL;
                        if (vehiclePool->isValid(i)) {
                            uintptr_t address = vehiclePool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _handles.push_back(rage::AddressToEntity(address));
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Ped)) {
                if (*_pedPoolAddress) {
                    GenericPool* pedPool = reinterpret_cast<GenericPool*>(*_pedPoolAddress);

                    for (int i = 0; i < pedPool->size; i++) {
                        CHECK_ENTITY_POOL;
                        if (pedPool->isValid(i)) {
                            uintptr_t address = pedPool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _handles.push_back(rage::AddressToEntity(address));
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Object)) {
                if (*_objectPoolAddress) {
                    GenericPool* propPool = reinterpret_cast<GenericPool*>(*_objectPoolAddress);

                    for (int i = 0; i < propPool->size; i++) {
                        CHECK_ENTITY_POOL;
                        if (propPool->isValid(i)) {
                            uintptr_t address = propPool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _handles.push_back(rage::AddressToEntity(address));
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Object)) {
                if (*_cameraPoolAddress) {
                    GenericPool* propPool = reinterpret_cast<GenericPool*>(*_cameraPoolAddress);

                    for (int i = 0; i < propPool->size; i++) {
                        CHECK_ENTITY_POOL;
                        if (propPool->isValid(i)) {
                            uintptr_t address = propPool->getAddress(i);
                            if (address && CheckEntity(address)) {
                                _handles.push_back(rage::AddressToEntity(address));
                            }
                        }
                    }
                }
            }
            if (TypeHasFlag(Type::Pickup)) {
                if (*_pickupObjectPoolAddress) {
                    GenericPool* pickupPool = reinterpret_cast<GenericPool*>(*_pickupObjectPoolAddress);

                    for (int i = 0; i < pickupPool->size; i++) {
                        CHECK_ENTITY_POOL;
                        if (pickupPool->isValid(i)) {
                            uintptr_t address = pickupPool->getAddress(i);
                            if (address) {
                                _handles.push_back(rage::AddressToEntity(address));
                            }
                        }
                    }
                }
            }
        }

        UINT16 _type;
        bool _posCheck, _modelCheck;
        Vector3 _position;
        float _radiusSquared;
        std::vector<DWORD> _modelHashes;
    };
    struct GenericTask
    {
    public:
        typedef UINT64(*func)(UINT64);
        GenericTask(func pFunc, UINT64 Arg) : _toRun(pFunc), _arg(Arg)
        {
        }
        virtual void Run()
        {
            _res = _toRun(_arg);
        }

        UINT64 GetResult()
        {
            return _res;
        }

    private:
        func _toRun;
        UINT64 _arg;
        UINT64 _res;
    };

    std::string worldGetEntityPoolStats() {
        EntityPool* entityPool = reinterpret_cast<EntityPool*>(*_entityPoolAddress);

        return FMT("maxCount: %i  itemSize: %i  firstEmptySlot: %i  emptySlots: %i  free: %i",
            entityPool->maxCount,       // num1
            entityPool->itemSize,       // 16 bytes
            entityPool->firstEmptySlot, // not sure if these are valid for EntityPools -- sfink
            entityPool->emptySlots,     // not sure if these are valid for EntityPools -- sfink
            entityPool->maxCount - (entityPool->itemCount & 0x3FFFFFFF)
        );
    }

#define WGA_HAND(VEHICLE)                                              \
    void Get##VEHICLE##Handles(std::vector<uintptr_t>& result) {       \
        EntityPoolTask(EntityPoolTask::Type::##VEHICLE##).Run(result); \
    }                                                                  \
    void Get##VEHICLE##Handles(std::vector<int>& result)               \
{                                                                      \
    EntityPoolTask(EntityPoolTask::Type::##VEHICLE##).Run(result);     \
}

#define WGA_ENT(VEHICLE)                                               \
    std::vector<##VEHICLE##> worldGetAll##VEHICLE##s() {               \
        AllWorld##VEHICLE##s.clear();                                  \
        Get##VEHICLE##Handles(AllWorld##VEHICLE##s);                   \
        return recastVector<##VEHICLE##>(AllWorld##VEHICLE##s);        \
    }                                                                  \
    std::vector<int>& worldGetAll##VEHICLE##Ints() {                   \
        AllWorld##VEHICLE##s.clear();                                  \
        Get##VEHICLE##Handles(AllWorld##VEHICLE##s);                   \
        return AllWorld##VEHICLE##s;                                   \
    }                                                                  \
    std::vector<uintptr_t>& worldGetAll##VEHICLE##Pointers() {         \
        AllWorld##VEHICLE##Pointers.clear();                           \
        Get##VEHICLE##Handles(AllWorld##VEHICLE##Pointers);            \
        return AllWorld##VEHICLE##Pointers;                            \
    }                                                                  \
    int worldGetAll##VEHICLE##IntArray(int *arr, int arrSize) {        \
        int count = 0;                                                 \
        for (uintptr_t address : worldGetAll##VEHICLE##Pointers()) {   \
            if (count == arrSize) break;                               \
            int entity = rage::AddressToEntity(address);                   \
            if (entity) arr[count++] = entity;                         \
        }                                                              \
        return count;                                                  \
    }

#define WGA_MACRO(VEHICLE)                                             \
    std::vector<int> AllWorld##VEHICLE##s;                             \
    std::vector<uintptr_t> AllWorld##VEHICLE##Pointers;                \
    WGA_HAND(VEHICLE)                                                  \
    WGA_ENT(VEHICLE)

    WGA_MACRO(Vehicle);
    WGA_MACRO(Ped);
    WGA_MACRO(Object);
    WGA_MACRO(Pickup);
    WGA_MACRO(Camera);
};