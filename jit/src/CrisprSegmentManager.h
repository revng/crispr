//
// Created by fcremo on 10/30/20.
//

#ifndef CRISPR_CRISPRSEGMENTMANAGER_H
#define CRISPR_CRISPRSEGMENTMANAGER_H

#include <cstdint>

class CrisprSegmentManager {
private:
    uint64_t NextFreeCodeSegmentVirtualAddress;
    uint64_t NextFreeDataSegmentVirtualAddress;
    uint64_t NextFreeRoDataSegmentVirtualAddress;

public:
    CrisprSegmentManager(
            uint64_t baseCodeSegmentVirtualAddress,
            uint64_t baseDataSegmentVirtualAddress,
            uint64_t baseRoDataSegmentVirtualAddress) : NextFreeCodeSegmentVirtualAddress(baseCodeSegmentVirtualAddress),
                                                        NextFreeDataSegmentVirtualAddress(baseDataSegmentVirtualAddress),
                                                        NextFreeRoDataSegmentVirtualAddress(baseRoDataSegmentVirtualAddress) {}

public:
    uint64_t requestCodeAddr(uintptr_t size) {
        uint64_t ReturnedAddress = NextFreeCodeSegmentVirtualAddress;
        NextFreeCodeSegmentVirtualAddress += size;
        return ReturnedAddress;
    }

    uint64_t requestDataAddr(uintptr_t size) {
        uint64_t ReturnedAddress = NextFreeDataSegmentVirtualAddress;
        NextFreeDataSegmentVirtualAddress += size;
        return ReturnedAddress;
    }

    uint64_t requestRoDataAddr(uintptr_t size) {
        uint64_t ReturnedAddress = NextFreeRoDataSegmentVirtualAddress;
        NextFreeRoDataSegmentVirtualAddress += size;
        return ReturnedAddress;
    }
};

#endif//CRISPR_CRISPRSEGMENTMANAGER_H
