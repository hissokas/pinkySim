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

#include "pinkySimBaseTest.h"

TEST_GROUP_BASE(addImmediate, pinkySimBase)
{
    void setup()
    {
        pinkySimBase::setup();
    }

    void teardown()
    {
        pinkySimBase::teardown();
    }
};


/* ADD - Immediate - Encoding T1
   Encoding: 000 11 1 0 Imm:3 Rn:3 Rd:3 */
TEST(addImmediate, T1UseLowestRegisterOnlyAddSmallestImmediateWithZeroResult)
{
    emitInstruction16("0001110iiinnnddd", 0, R0, R0);
    setExpectedXPSRflags("nZcv");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(addImmediate, T1UseHigestRegisterOnlyAddLargestImmediate)
{
    emitInstruction16("0001110iiinnnddd", 7, R7, R7);
    setExpectedXPSRflags("nzcv");
    setExpectedRegisterValue(R7, 0x77777777U + 7U);
    pinkySimStep(&m_context);
}

TEST(addImmediate, T1UseDifferentRegistersForEachArgAdd3AndAllAPSRFlagsShouldBeClear)
{
    emitInstruction16("0001110iiinnnddd", 3, R2, R0);
    setExpectedXPSRflags("nzcv");
    setExpectedRegisterValue(R0, 0x22222222U + 3U);
    pinkySimStep(&m_context);
}

TEST(addImmediate, T1ForceCarryByAdding1ToLargestInteger)
{
    emitInstruction16("0001110iiinnnddd", 1, R7, R0);
    setExpectedXPSRflags("nZCv");
    setRegisterValue(R7, 0xFFFFFFFFU);
    setExpectedRegisterValue(R0, 0);
    pinkySimStep(&m_context);
}

TEST(addImmediate, T1ForceOverflowPastLargestPositiveInteger)
{
    emitInstruction16("0001110iiinnnddd", 1, R0, R7);
    setExpectedXPSRflags("NzcV");
    setRegisterValue(R0, 0x7FFFFFFFU);
    setExpectedRegisterValue(R7, 0x7FFFFFFFU + 1);
    pinkySimStep(&m_context);
}



/* ADD - Immediate - Encoding T2
   Encoding: 001 10 Rdn:3 Imm:8 */
TEST(addImmediate, T2UseLowestRegisterAndAddSmallestImmediateWithZeroResult)
{
    emitInstruction16("00110dddiiiiiiii", R0, 0);
    setExpectedXPSRflags("nZcv");
    setExpectedRegisterValue(R0, 0U);
    pinkySimStep(&m_context);
}

TEST(addImmediate, T2UseHigestRegisterAndAddLargestImmediate)
{
    emitInstruction16("00110dddiiiiiiii", R7, 255);
    setExpectedXPSRflags("nzcv");
    setExpectedRegisterValue(R7, 0x77777777U + 255U);
    pinkySimStep(&m_context);
}

TEST(addImmediate, T2ForceCarryByAdding1ToLargestInteger)
{
    emitInstruction16("00110dddiiiiiiii", R3, 1);
    setExpectedXPSRflags("nZCv");
    setRegisterValue(R3, 0xFFFFFFFFU);
    setExpectedRegisterValue(R3, 0);
    pinkySimStep(&m_context);
}

TEST(addImmediate, T2ForceOverflowPastLargestPositiveInteger)
{
    emitInstruction16("00110dddiiiiiiii", R3, 1);
    setExpectedXPSRflags("NzcV");
    setRegisterValue(R3, 0x7FFFFFFFU);
    setExpectedRegisterValue(R3, 0x7FFFFFFFU + 1);
    pinkySimStep(&m_context);
}
