/*  Copyright (C) 2014  Adam Green (https://github.com/adamgreen)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
// Include headers from C modules under test.
extern "C"
{
    #include <MemorySim.h>
    #include <MallocFailureInject.h>
}

// Include C++ headers for test harness.
#include "CppUTest/TestHarness.h"


TEST_GROUP(MemorySim)
{
    IMemory* m_pMemory;
    
    void setup()
    {
        m_pMemory = MemorySim_Init();
    }

    void teardown()
    {
        CHECK_EQUAL(noException, getExceptionCode());
        clearExceptionCode();
        MemorySim_Uninit(m_pMemory);
        MallocFailureInject_Restore();
    }
    
    void validateExceptionThrown(int expectedExceptionCode)
    {
        CHECK_EQUAL(expectedExceptionCode, getExceptionCode());
        clearExceptionCode();
    }
};

TEST(MemorySim, BasicInitTakenCareOfInSetup)
{
    CHECK(m_pMemory != NULL);
}

TEST(MemorySim, Uninit_ShouldHandleNULLPointer)
{
    MemorySim_Uninit(NULL);
}

TEST(MemorySim, NoMemoryRegionsSetupShouldResultInAllReadsAndWritesThrowing)
{
    __try_and_catch( IMemory_Read32(m_pMemory, 0x00000000) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, 0x00000000) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read8(m_pMemory, 0x00000000) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write32(m_pMemory, 0x00000000, 0xFFFFFFFF) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, 0x00000000, 0xFFFF) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, 0x00000000, 0xFF) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, ShouldThrowIfOutOfMemory)
{
    // Each region has two allocations.
    // 1. The MemoryRegion structure which describes the region.
    // 2. The array of bytes used to simulate the memory.
    static const size_t allocationsToFail = 2;
    size_t i;
    
    for (i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( MemorySim_CreateRegion(m_pMemory, 0x00000004, 4) );
        validateExceptionThrown(outOfMemoryException);
    }
    MallocFailureInject_FailAllocation(i);
    MemorySim_CreateRegion(m_pMemory, 0x00000004, 4);
}

TEST(MemorySim, SimulateFourBytes_ShouldBeZeroFilledByDefault)
{
    MemorySim_CreateRegion(m_pMemory, 0x00000004, 4);
    CHECK_EQUAL(0x0000000, IMemory_Read32(m_pMemory, 0x00000004));
}

TEST(MemorySim, SimulateFourBytes_DefaultsToReadWrite_VerifyCanReadAndWrite)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    IMemory_Write32(m_pMemory, testAddress, 0x11111111);
    CHECK_EQUAL(0x11111111, IMemory_Read32(m_pMemory, testAddress));
    IMemory_Write16(m_pMemory, testAddress, 0x2222);
    CHECK_EQUAL(0x2222, IMemory_Read16(m_pMemory, testAddress));
    IMemory_Write8(m_pMemory, testAddress, 0x33);
    CHECK_EQUAL(0x33, IMemory_Read8(m_pMemory, testAddress));
}

TEST(MemorySim, SimulateFourBytes_VerifyReadWritesOfPreviousWordThrows)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress - 4, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read32(m_pMemory, testAddress - 4) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress - 2, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, testAddress - 2) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, testAddress - 1, 0x33) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read8(m_pMemory, testAddress - 1) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, SimulateFourBytes_VerifyReadWritesOfNextWordThrows)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress + 4, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read32(m_pMemory, testAddress + 4) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress + 4, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, testAddress + 4) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, testAddress + 4, 0x33) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read8(m_pMemory, testAddress + 4) );
    validateExceptionThrown(busErrorException);
}


TEST(MemorySim, SimulateFourBytes_VerifyOverlappingReadWritesWillThrow)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress + 1, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read32(m_pMemory, testAddress + 1) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress + 3, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Read16(m_pMemory, testAddress + 3) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, SimulateFourBytes_VerifyCanMakeReadOnly)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);
    MemorySim_MakeRegionReadOnly(m_pMemory, testAddress);

    __try_and_catch( IMemory_Write32(m_pMemory, testAddress, 0x11111111) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write16(m_pMemory, testAddress, 0x2222) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write8(m_pMemory, testAddress, 0x33) );
    validateExceptionThrown(busErrorException);

    CHECK_EQUAL(0x00000000, IMemory_Read32(m_pMemory, testAddress));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testAddress));
    CHECK_EQUAL(0x00, IMemory_Read8(m_pMemory, testAddress));
}

TEST(MemorySim, SimulateFourBytes_VerifyCanReadBothHalfWords)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    IMemory_Write32(m_pMemory, testAddress, 0x12345678);
    CHECK_EQUAL(0x5678, IMemory_Read16(m_pMemory, testAddress));
    CHECK_EQUAL(0x1234, IMemory_Read16(m_pMemory, testAddress + 2));
}

TEST(MemorySim, SimulateFourBytes_VerifyCanReadAllFourBytes)
{
    static const uint32_t testAddress = 0x00000004;
    MemorySim_CreateRegion(m_pMemory, testAddress, 4);

    IMemory_Write32(m_pMemory, testAddress, 0x12345678);
    CHECK_EQUAL(0x78, IMemory_Read8(m_pMemory, testAddress));
    CHECK_EQUAL(0x56, IMemory_Read8(m_pMemory, testAddress + 1));
    CHECK_EQUAL(0x34, IMemory_Read8(m_pMemory, testAddress + 2));
    CHECK_EQUAL(0x12, IMemory_Read8(m_pMemory, testAddress + 3));
}

TEST(MemorySim, SimulateLargerRegion_WriteReadAllWords)
{
    static const uint32_t baseAddress = 0x10000000;
    static const uint32_t size = 64 * 1024;
    static const uint32_t wordCount = size / sizeof(uint32_t);
    uint32_t i;
    uint32_t testValue;
    uint32_t address;
    
    MemorySim_CreateRegion(m_pMemory, baseAddress, size);

    for (i = 0, address = baseAddress, testValue = 0xFFFFFFFF ; i < wordCount ; i++, testValue--, address += 4)
        IMemory_Write32(m_pMemory, address, testValue);

    for (i = 0, address = baseAddress, testValue = 0xFFFFFFFF ; i < wordCount ; i++, testValue--, address += 4)
        CHECK_EQUAL(testValue, IMemory_Read32(m_pMemory, address));
}

TEST(MemorySim, SimulateTwoMemoryRegions)
{
    static const uint32_t region1 = 0x00000000;
    static const uint32_t region2 = 0x10000000;
    MemorySim_CreateRegion(m_pMemory, region1, 4);
    MemorySim_CreateRegion(m_pMemory, region2, 4);

    IMemory_Write32(m_pMemory, region1, 0x11111111);
    CHECK_EQUAL(0x11111111, IMemory_Read32(m_pMemory, region1));
    IMemory_Write32(m_pMemory, region2, 0x22222222);
    CHECK_EQUAL(0x22222222, IMemory_Read32(m_pMemory, region2));
}

TEST(MemorySim, CreateRegionsBasedOnFlashImage_TwoWordsOfFLASH_OneWordOfRAM_CheckRanges)
{
    uint32_t flashBinary[2] = { 0x10000004, 0x00000200 };
    MemorySim_CreateRegionsFromFlashImage(m_pMemory, flashBinary, sizeof(flashBinary));

    // FLASH is read-only from 0x00000000 - 0x00000007
    CHECK_EQUAL(flashBinary[0], IMemory_Read32(m_pMemory, FLASH_BASE_ADDRESS));
    CHECK_EQUAL(flashBinary[1], IMemory_Read32(m_pMemory, FLASH_BASE_ADDRESS + 4));
    __try_and_catch( IMemory_Read32(m_pMemory, FLASH_BASE_ADDRESS + 8) );
    validateExceptionThrown(busErrorException);

    __try_and_catch( IMemory_Write32(m_pMemory, FLASH_BASE_ADDRESS, 0x12345678) );
    validateExceptionThrown(busErrorException);
    __try_and_catch( IMemory_Write32(m_pMemory, FLASH_BASE_ADDRESS + 4, 0x12345678) );
    validateExceptionThrown(busErrorException);
    
    // RAM is read-write from 0x10000000 - 0x10000003
    IMemory_Write32(m_pMemory, 0x10000000, 0x12345678);
    CHECK_EQUAL(0x12345678, IMemory_Read32(m_pMemory, 0x10000000));
    __try_and_catch( IMemory_Read32(m_pMemory, 0x10000004) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, CreateRegionsBasedOnFlashImage_NonWordSizedFlash)
{
    uint32_t flashBinary[2] = { 0x10000004, 0x00000200 };
    MemorySim_CreateRegionsFromFlashImage(m_pMemory, flashBinary, sizeof(flashBinary) - 1);

    for (uint32_t i = 0 ; i < sizeof(flashBinary) - 1; i++)
        CHECK_EQUAL(((uint8_t*)flashBinary)[i], IMemory_Read8(m_pMemory, FLASH_BASE_ADDRESS + i));

    __try_and_catch( IMemory_Read8(m_pMemory, FLASH_BASE_ADDRESS + 7) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, CreateRegionsFromFlashImage_ShouldThrowIfOutOfMemory)
{
    // Each region has two allocations:
    // 1. The MemoryRegion structure which describes the region.
    // 2. The array of bytes used to simulate the memory.
    // This API creates two regions (FLASH and RAM) so there are a total of 4 allocations.
    static const size_t allocationsToFail = 4;
    uint32_t            flashBinary[2] = { 0x10000004, 0x00000200 };
    size_t              i;
    
    for (i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( MemorySim_CreateRegionsFromFlashImage(m_pMemory, flashBinary, sizeof(flashBinary)) );
        validateExceptionThrown(outOfMemoryException);
    }
    MallocFailureInject_FailAllocation(i);
    MemorySim_CreateRegionsFromFlashImage(m_pMemory, flashBinary, sizeof(flashBinary));
}

TEST(MemorySim, CreateRegionsFromFlashImage_MakeSureItHandlesOutOfMemoryWhenThereIsAnExistingRegion)
{
    MemorySim_CreateRegion(m_pMemory, 0xF0000000, 4);
    IMemory_Write32(m_pMemory, 0xF0000000, 0x12345678);
    
    // Each region has two allocations:
    // 1. The MemoryRegion structure which describes the region.
    // 2. The array of bytes used to simulate the memory.
    // This API creates two regions (FLASH and RAM) so there are a total of 4 allocations.
    static const size_t allocationsToFail = 4;
    uint32_t            flashBinary[2] = { 0x10000004, 0x00000200 };
    size_t              i;
    
    for (i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( MemorySim_CreateRegionsFromFlashImage(m_pMemory, flashBinary, sizeof(flashBinary)) );
        validateExceptionThrown(outOfMemoryException);
    }
    
    // The manually created region should still be valid.
    CHECK_EQUAL(0x12345678, IMemory_Read32(m_pMemory, 0xF0000000));
}

TEST(MemorySim, CreateRegionsBasedOnFlashImage_ShouldThrowIfNotBigEnoughForInitialStackPointer)
{
    uint32_t flashBinary = 0x10000004;
    __try_and_catch( MemorySim_CreateRegionsFromFlashImage(m_pMemory, &flashBinary, sizeof(flashBinary) - 1) );
    validateExceptionThrown(bufferOverrunException);
}



TEST(MemorySim, AttemptToSetBreakpointAtInvalidAddress_ShouldThrow)
{
    uint32_t testBase = 0x00000000;
    __try_and_catch( MemorySim_SetHardwareBreakpoint(m_pMemory, testBase, sizeof(uint16_t)) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, AttemptToClearBreakpointAtInvalidAddress_ShouldThrow)
{
    uint32_t testBase = 0x00000000;
    __try_and_catch( MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase, sizeof(uint16_t)) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, SetBreakpointShouldThrowIfOutOfMemory)
{
    static const size_t allocationsToFail = 1;
    uint32_t            testBase = 0x00000000;
    size_t              i;
    
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    for (i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( MemorySim_SetHardwareBreakpoint(m_pMemory, testBase, sizeof(uint16_t)) );
        validateExceptionThrown(outOfMemoryException);
    }
    MallocFailureInject_FailAllocation(i);
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase, sizeof(uint16_t));
}

TEST(MemorySim, SetAndClear2ByteHardwareBreakpoint_IssueReadsWhichHitAndMissBreakpoint)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)));

    MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
}

TEST(MemorySim, SetWordSizedHardwareBreakpoint_HitOnHalfWordAccessWithinRange)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint32_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint32_t), sizeof(uint32_t));
    __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint32_t)) );
    validateExceptionThrown(hardwareBreakpointException);
    __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint32_t) + sizeof(uint16_t)) );
    validateExceptionThrown(hardwareBreakpointException);
}

TEST(MemorySim, SetWordSizedHardwareBreakpoint_ShouldIgnoreWordAccesses)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint32_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint32_t), sizeof(uint32_t));
    CHECK_EQUAL(0x0000, IMemory_Read32(m_pMemory, testBase + 0 * sizeof(uint32_t)));
    CHECK_EQUAL(0x0000, IMemory_Read32(m_pMemory, testBase + 1 * sizeof(uint32_t)));
    CHECK_EQUAL(0x0000, IMemory_Read32(m_pMemory, testBase + 2 * sizeof(uint32_t)));
}

TEST(MemorySim, HardwareBreakpoint_NoHitOnWrites)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    IMemory_Write16(m_pMemory, testBase + 0 * sizeof(uint16_t), 0x1234);
    IMemory_Write16(m_pMemory, testBase + 1 * sizeof(uint16_t), 0x1234);
    IMemory_Write16(m_pMemory, testBase + 2 * sizeof(uint16_t), 0x1234);
}

TEST(MemorySim, SetAndVerifyTwoHardwareBreakpoints_SetInAscendingAddressOrder)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 4 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 2 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 3 * sizeof(uint16_t)));
}

TEST(MemorySim, SetAndVerifyTwoHardwareBreakpoints_SetInDescendingAddressOrder)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 4 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 2 * sizeof(uint16_t), sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 3 * sizeof(uint16_t)));
}

TEST(MemorySim, SetAndClearTwoHardwareBreakpoints_ClearInSameOrderAsSet)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 4 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 2 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 3 * sizeof(uint16_t)));

    // Clear first breakpoint.
    MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
    __try_and_catch( IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)) );
    validateExceptionThrown(hardwareBreakpointException);

    // Clear second breakpoint.
    MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase + 2 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)));
}

TEST(MemorySim, SetAndClearTwoHardwareBreakpoints_ClearInOppositeOrderAsSet)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 4 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 2 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 3 * sizeof(uint16_t)));

    // Clear second breakpoint.
    MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase + 2 * sizeof(uint16_t), sizeof(uint16_t));
    __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)) );
    validateExceptionThrown(hardwareBreakpointException);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)));

    // Clear first breakpoint.
    MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)));
}

TEST(MemorySim, SetAndVerifyThreeHardwareBreakpoints)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 5 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 2 * sizeof(uint16_t), sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 3 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
        __try_and_catch( IMemory_Read16(m_pMemory, testBase + 3 * sizeof(uint16_t)) );
        validateExceptionThrown(hardwareBreakpointException);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 4 * sizeof(uint16_t)));
}

TEST(MemorySim, SetSameBreakpointTwice_SecondShouldBeIgnored)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 1 * sizeof(uint16_t));
    
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase, sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase, sizeof(uint16_t));

    __try_and_catch( IMemory_Read16(m_pMemory, testBase) );
    validateExceptionThrown(hardwareBreakpointException);

    // Clear breakpoint once and it should now be completely cleared.
    MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase, sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase));
}

TEST(MemorySim, TryClearingBreakpointWhichNoExist_ShouldBeIgnored)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 2 * sizeof(uint16_t));
    MemorySim_SetHardwareBreakpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t));

    MemorySim_ClearHardwareBreakpoint(m_pMemory, testBase + 0 * sizeof(uint16_t), sizeof(uint16_t));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
}

TEST(MemorySim, SetAndVerifySeveralBreakpoints)
{
    uint32_t startAddress = 0x00000000;
    uint32_t size = 4 * 1024;
    uint32_t endAddress = startAddress + size;
    uint32_t address;
    
    MemorySim_CreateRegion(m_pMemory, startAddress, size);
    
    for (address = startAddress ; address < endAddress ; address += 2 * sizeof(uint16_t))
        MemorySim_SetHardwareBreakpoint(m_pMemory, address, sizeof(uint16_t));
    
    for (address = startAddress + sizeof(uint16_t) ; address < endAddress ; address += 2 * sizeof(uint16_t))
        CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, address));

    for (address = startAddress ; address < endAddress ; address += 2 * sizeof(uint16_t))
    {
        __try_and_catch( IMemory_Read16(m_pMemory, address) );
        validateExceptionThrown(hardwareBreakpointException);
    }
}

TEST(MemorySim, AttemptToSetWatchpointAtInvalidAddress_ShouldThrow)
{
    uint32_t testBase = 0x00000000;
    __try_and_catch( MemorySim_SetHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, AttemptToClearWatchpointAtInvalidAddress_ShouldThrow)
{
    uint32_t testBase = 0x00000000;
    __try_and_catch( MemorySim_ClearHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ) );
    validateExceptionThrown(busErrorException);
}

TEST(MemorySim, SetWatchpointShouldThrowIfOutOfMemory)
{
    static const size_t allocationsToFail = 1;
    uint32_t            testBase = 0x00000000;
    size_t              i;
    
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    for (i = 1 ; i <= allocationsToFail ; i++)
    {
        MallocFailureInject_FailAllocation(i);
        __try_and_catch( MemorySim_SetHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ) );
        validateExceptionThrown(outOfMemoryException);
    }
    MallocFailureInject_FailAllocation(i);
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ);
}

TEST(MemorySim, SetAndClear2ByteHardwareWatchpoint_IssueReadsWhichHitAndMissWatchpoint)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_READ);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
        CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
        CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
        CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)));

    MemorySim_ClearHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_READ);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, SetAndClear4ByteHardwareWatchpoint_IssueReadsWhichHitAndMissBreakpoint)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint32_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint32_t), sizeof(uint32_t), WATCHPOINT_READ);
    CHECK_EQUAL(0x00000000, IMemory_Read32(m_pMemory, testBase + 0 * sizeof(uint32_t)));
        CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
        CHECK_EQUAL(0x0000000, IMemory_Read32(m_pMemory, testBase + 1 * sizeof(uint32_t)));
        CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
    CHECK_EQUAL(0x00000000, IMemory_Read32(m_pMemory, testBase + 2 * sizeof(uint32_t)));

    MemorySim_ClearHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint32_t), sizeof(uint32_t), WATCHPOINT_READ);
    CHECK_EQUAL(0x00000000, IMemory_Read32(m_pMemory, testBase + 1 * sizeof(uint32_t)));
}

TEST(MemorySim, Set4ByteHardwareWatchpoint_HitOnSmallerSize)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint32_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint32_t), sizeof(uint32_t), WATCHPOINT_READ);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint32_t)));
    CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));

    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint32_t) + sizeof(uint16_t)));
    CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, HardwareReadWatchpoint_NoHitOnWrites)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_READ);
    IMemory_Write16(m_pMemory, testBase + 0 * sizeof(uint16_t), 0x1234);
    IMemory_Write16(m_pMemory, testBase + 1 * sizeof(uint16_t), 0x1234);
    IMemory_Write16(m_pMemory, testBase + 2 * sizeof(uint16_t), 0x1234);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, HardwareWriteWatchpoint_NoHitOnReads)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_WRITE);
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
    CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)));
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, HardwareWriteWatchpoint_HitOnWrites)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_WRITE);
    IMemory_Write16(m_pMemory, testBase + 0 * sizeof(uint16_t), 0x1234);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
        IMemory_Write16(m_pMemory, testBase + 1 * sizeof(uint16_t), 0x1234);
        CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
    IMemory_Write16(m_pMemory, testBase + 2 * sizeof(uint16_t), 0x1234);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, HardwareReadWriteWatchpoint_HitsOnReadsAndWrites)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 3 * sizeof(uint16_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_READ_WRITE);
    IMemory_Write16(m_pMemory, testBase + 0 * sizeof(uint16_t), 0x1234);
    CHECK_EQUAL(0x1234, IMemory_Read16(m_pMemory, testBase + 0 * sizeof(uint16_t)));
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
        IMemory_Write16(m_pMemory, testBase + 1 * sizeof(uint16_t), 0x1234);
        CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
        CHECK_EQUAL(0x1234, IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t)));
        CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
    IMemory_Write16(m_pMemory, testBase + 2 * sizeof(uint16_t), 0x1234);
    CHECK_EQUAL(0x1234, IMemory_Read16(m_pMemory, testBase + 2 * sizeof(uint16_t)));
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, SetSameWatchpointTwice_SecondShouldBeIgnored)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 1 * sizeof(uint16_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ);
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ);

    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    IMemory_Read16(m_pMemory, testBase);
    CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));

    // Clear breakpoint once and it should now be completely cleared.
    MemorySim_ClearHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ);
    IMemory_Read16(m_pMemory, testBase);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, SetTwoDifferentWatchpointTypesAtSameAddress_BothShouldBeObserved)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 1 * sizeof(uint16_t));
    
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_READ);
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase, sizeof(uint16_t), WATCHPOINT_WRITE);

    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    IMemory_Write16(m_pMemory, testBase, 0x1234);
    CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
    IMemory_Read16(m_pMemory, testBase);
    CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, TryClearingWatchpointWhichNoExist_ShouldBeIgnored)
{
    uint32_t testBase = 0x00000000;
    MemorySim_CreateRegion(m_pMemory, testBase, 2 * sizeof(uint16_t));
    MemorySim_SetHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_READ);

    MemorySim_ClearHardwareWatchpoint(m_pMemory, testBase + 1 * sizeof(uint16_t), sizeof(uint16_t), WATCHPOINT_WRITE);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    IMemory_Write16(m_pMemory, testBase + 1 * sizeof(uint16_t), 0x1234);
    CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    IMemory_Read16(m_pMemory, testBase + 1 * sizeof(uint16_t));
    CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
}

TEST(MemorySim, SetAndVerifySeveralWatchpoints)
{
    uint32_t startAddress = 0x00000000;
    uint32_t size = 4 * 1024;
    uint32_t endAddress = startAddress + size;
    uint32_t address;
    
    MemorySim_CreateRegion(m_pMemory, startAddress, size);
    
    for (address = startAddress ; address < endAddress ; address += 2 * sizeof(uint16_t))
        MemorySim_SetHardwareWatchpoint(m_pMemory, address, sizeof(uint16_t), WATCHPOINT_READ);
    
    for (address = startAddress + sizeof(uint16_t) ; address < endAddress ; address += 2 * sizeof(uint16_t))
    {
        CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, address));
        CHECK_FALSE(MemorySim_WasWatchpointEncountered(m_pMemory));
    }

    for (address = startAddress ; address < endAddress ; address += 2 * sizeof(uint16_t))
    {
        CHECK_EQUAL(0x0000, IMemory_Read16(m_pMemory, address));
        CHECK_TRUE(MemorySim_WasWatchpointEncountered(m_pMemory));
    }
}
