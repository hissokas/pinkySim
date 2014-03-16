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

TEST_GROUP_BASE(mvnRegister, pinkySimBase)
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


/* MVN - Register (MOve Negative)
   Encoding: 010000 1111 Rm:3 Rd:3 */
TEST(mvnRegister, UseR1ForAllArgs)
{
    emitInstruction16("0100001111mmmddd", R1, R1);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R1, ~0x11111111U);
    pinkySimStep(&m_context);
}

TEST(mvnRegister, UseLowestRegisterForAllArgs)
{
    emitInstruction16("0100001111mmmddd", R0, R0);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R0, ~0U);
    pinkySimStep(&m_context);
}

TEST(mvnRegister, UseHigestRegisterForAllArgs)
{
    emitInstruction16("0100001111mmmddd", R7, R7);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R7, ~0x77777777U);
    pinkySimStep(&m_context);
}

TEST(mvnRegister, UseDifferentRegistersForEachArg)
{
    emitInstruction16("0100001111mmmddd", R2, R1);
    setExpectedXPSRflags("Nz");
    setExpectedRegisterValue(R1, ~0x22222222U);
    pinkySimStep(&m_context);
}

TEST(mvnRegister, MoveANegationOfNegativeOneWhichClearsNegativeFlagAndSetsZeroFlag)
{
    emitInstruction16("0100001111mmmddd", R2, R1);
    setRegisterValue(R2, -1);
    setExpectedXPSRflags("nZ");
    setExpectedRegisterValue(R1, 0U);
    pinkySimStep(&m_context);
}
