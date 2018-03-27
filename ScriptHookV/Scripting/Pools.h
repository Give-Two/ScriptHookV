#pragma once
#include "dassert.h"

namespace internal {

    enum eCEntityType : UINT8 {
        entityTypeCVehicle = 3,
        entityTypeCPed,
        entityTypeCObject
    };

    struct CBaseModelInfo
    {
        void *vtable;
        // ...
    };

    struct CEntity {
        void *vtable;
        int entityIndex;
        int field_C;
        __int64 SLL[2];
        CBaseModelInfo *modelInfo;
        eCEntityType     entityType;
    };

    // For recasting a Vector of Entity/Object/Ped/Pickup/Vehicle
    template <typename T, typename U>
    std::vector<T> recastVector(std::vector<U> v1) {
        std::vector<T> v2(v1.begin(), v1.end());
        return v2;
    }

    typedef int Camera;
    void InitPools();

    std::string worldGetEntityPoolStats();

    std::vector<Vehicle>    worldGetAllVehicles();
    std::vector<Ped>        worldGetAllPeds();
    std::vector<Object>     worldGetAllObjects();
    std::vector<Pickup>     worldGetAllPickups();
    std::vector<Camera>     worldGetAllCameras();

    std::vector<int>&       worldGetAllVehicleInts();
    std::vector<int>&       worldGetAllPedInts();
    std::vector<int>&       worldGetAllObjectInts();
    std::vector<int>&       worldGetAllPickupInts();
    std::vector<int>&       worldGetAllCameraInts();

    std::vector<uintptr_t>& worldGetAllVehiclePointers();
    std::vector<uintptr_t>& worldGetAllPedPointers();
    std::vector<uintptr_t>& worldGetAllObjectPointers();
    std::vector<uintptr_t>& worldGetAllPickupPointers();
    std::vector<uintptr_t>& worldGetAllCameraPointers();

    // SHV compatible pool interfaces
    int worldGetAllVehicleIntArray(int*, int);
    int worldGetAllPedIntArray(int*, int);
    int worldGetAllObjectIntArray(int*, int);
	int worldGetAllPickupIntArray(int*, int);
    int worldGetAllCameraIntArray(int*, int);
};
