/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <iostream>
#include <sstream>
#include <string>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"

using namespace CLRX;

struct AsmRegVarUsageData
{
    size_t offset;
    const char* regVarName;
    uint16_t rstart, rend;
    AsmRegField regField;
    cxbyte rwFlags;
    cxbyte align;
};

struct GCNRegVarUsageCase
{
    const char* input;
    Array<AsmRegVarUsageData> regVarUsages;
    bool good;
    const char* errorMessages;
};

static const GCNRegVarUsageCase gcnRvuTestCases1Tbl[] =
{
    {   /* SOP1 encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        "s_mov_b32 rax,rbx\n"
        ".space 12\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        ".space 134\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n",
        {
            // s_mov_b32 rax,rbx
            { 0, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 16, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 16, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 154, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 154, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
        },
        true, ""
    },
    {   /* SOP1 encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        ".space 200\n"
        "s_mov_b32 rax,rbx\n"
        ".space 12\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        ".space 134\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n",
        {
            // s_mov_b32 rax,rbx
            { 200, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 200, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 216, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 216, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 354, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 354, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
        },
        true, ""
    },
    {   /* SOP1 encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        "s_mov_b32 rax,rbx\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n"
        "s_ff1_i32_b64 rbx, rbx5[1:2]\n"
        "s_bitset0_b64 rbx5[3:4],rax\n"
        "s_getpc_b64 rax4[0:1]\n"
        "s_setpc_b64 rax4[2:3]\n"
        "s_cbranch_join rax4[2]\n"
        "s_movrels_b32 rax,rbx\n"
        "s_mov_b32 s23,s31\n"
        "s_mov_b64 s[24:25],s[42:43]\n",
        {
            // s_mov_b32 rax,rbx
            { 0, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 4, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 4, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 8, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_ff1_i32_b64 rbx, rbx5[1:2]
            { 12, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 12, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_bitset0_b64 rbx5[3:4],rax
            { 16, "rbx5", 3, 5, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 16, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_getpc_b64 rax4[0:1]
            { 20, "rax4", 0, 2, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            // s_setpc_b64 rax4[2:3]
            { 24, "rax4", 2, 4, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_cbranch_join rax4[2]
            { 28, "rax4", 2, 3, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_movrels_b32 rax,rbx
            { 32, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 32, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 s23,s31
            { 36, nullptr, 23, 24, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 36, nullptr, 31, 32, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            // s_mov_b64 s[24:25],s[42:43]
            { 40, nullptr, 24, 26, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 40, nullptr, 42, 44, GCNFIELD_SSRC0, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* SOP2 encoding */
        ".regvar rax:s, rbx:s, rdx:s\n"
        ".regvar rax4:s:8, rbx5:s:8, rcx3:s:6\n"
        "s_and_b32 rdx, rax, rbx\n"
        "s_or_b32 rdx, s11, rbx\n"
        "s_xor_b64 rcx3[4:5], rax4[0:1], rbx5[2:3]\n"
        "s_cbranch_g_fork  rcx3[0:1], rax4[2:3]\n"
        "s_and_b32 s46, s21, s62\n"
        "s_xor_b64 s[26:27], s[38:39], s[12:13]\n",
        {
            // s_and_b32 rdx, rax, rbx
            { 0, "rdx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_or_b32 rdx, s11, rbx
            { 4, "rdx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 4, nullptr, 11, 12, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 4, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_xor_b64 rcx3[4:5], rax4[0:1], rbx5[2:3]
            { 8, "rcx3", 4, 6, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rax4", 0, 2, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 8, "rbx5", 2, 4, GCNFIELD_SSRC1, ASMRVU_READ, 2 },
            // s_cbranch_g_fork  rcx3[0:1], rax4[2:3]
            { 12, "rcx3", 0, 2, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 12, "rax4", 2, 4, GCNFIELD_SSRC1, ASMRVU_READ, 2 },
            // s_and_b32 s46, s21, s62
            { 16, nullptr, 46, 47, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 16, nullptr, 21, 22, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 16, nullptr, 62, 63, GCNFIELD_SSRC1, ASMRVU_READ, 0 },
            // s_xor_b64 s[26:27], s[38:39], s[12:13]
            { 20, nullptr, 26, 28, GCNFIELD_SDST, ASMRVU_WRITE, 0 },
            { 20, nullptr, 38, 40, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 20, nullptr, 12, 14, GCNFIELD_SSRC1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* SOPC encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:4, rbx5:s:4\n"
        "s_cmp_ge_i32  rax, rbx\n"
        "s_bitcmp0_b64  rbx5[2:3], rax4[3]\n"
        "s_setvskip  rax, rbx5[2]\n"
        "s_cmp_ge_i32  s53, s9\n",
        {
            // s_cmp_ge_i32  rax, rbx
            { 0, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_bitcmp0_b64  rbx5[2:3], rax[3]
            { 4, "rbx5", 2, 4, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 4, "rax4", 3, 4, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_set_vskip  rax, rbx5[2]
            { 8, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 8, "rbx5", 2, 3, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_cmp_ge_i32  s53, s9
            { 12, nullptr, 53, 54, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 12, nullptr, 9, 10, GCNFIELD_SSRC1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* SOPK */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:4, rbx5:s:4\n"
        "s_cmpk_eq_i32  rbx, 0xd3b9\n"
        "s_addk_i32  rax, 0xd3b9\n"
        "s_cbranch_i_fork rbx5[2:3], xxxx-8\nxxxx:\n"
        "s_getreg_b32 rbx, hwreg(trapsts, 0, 1)\n"
        "s_setreg_b32  hwreg(trapsts, 3, 10), rax\n"
        "s_cmpk_eq_i32  s17, 0xd3b9\n",
        {
            // s_cmpk_eq_i32  rbx, 0xd3b9
            { 0, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_READ, 1 },
            // s_addk_i32  rax, 0xd3b9
            { 4, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            // s_cbranch_i_fork rbx5[2:3], xxxx-8
            { 8, "rbx5", 2, 4, GCNFIELD_SDST, ASMRVU_READ, 2 },
            // s_getreg_b32 rbx, hwreg(trapsts, 0, 1)
            { 12, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            // s_setreg_b32  hwreg(trapsts, 3, 10), rax
            { 16, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_READ, 1 },
            // s_cmpk_eq_i32  s17, 0xd3b9
            { 20, nullptr, 17, 18, GCNFIELD_SDST, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* SMRD */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:20, rbx5:s:16\n"
        "s_load_dword rbx, rbx5[2:3], 0x5b\n"
        "s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b\n"
        "s_load_dwordx4 rax4[0:3], rbx5[6:7], 0x5b\n"
        "s_load_dwordx8 rax4[0:7], rbx5[8:9], 0x5b\n"
        "s_load_dwordx16 rax4[4:19], rbx5[10:11], 0x5b\n"
        "s_load_dword rbx, rbx5[2:3], rbx5[6]\n"
        "s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b\n"
        "s_memtime  rax4[2:3]\n"
        "s_dcache_inv\n"
        "s_load_dwordx2 s[28:29], s[36:37], 0x5b\n"
        "s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b\n",
        {
            // s_load_dword rbx, rbx5[2:3], 0x5b
            { 0, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b
            { 4, "rax4", 0, 2, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            { 4, "rbx5", 4, 6, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx4 rax4[0:3], rbx5[4:5], 0x5b
            { 8, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 8, "rbx5", 6, 8, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx8 rax4[0:7], rbx5[4:5], 0x5b
            { 12, "rax4", 0, 8, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 12, "rbx5", 8, 10, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx16 rax4[4:19], rbx5[4:5], 0x5b
            { 16, "rax4", 4, 20, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 16, "rbx5", 10, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dword rbx, rbx5[2:3], rbx5[6]
            { 20, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 20, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            { 20, "rbx5", 6, 7, GCNFIELD_SMRD_SOFFSET, ASMRVU_READ, 1 },
            // s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b
            { 24, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 24, "rbx5", 8, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_memtime  rax4[2:3]
            { 28, "rax4", 2, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            // s_load_dwordx2 s[28:29], s[36:37], 0x5b
            { 36, nullptr, 28, 30, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 36, nullptr, 36, 38, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 },
            // s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b
            { 40, nullptr, 44, 48, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 40, nullptr, 12, 16, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* SMEM */
        ".gpu Fiji\n"
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:20, rbx5:s:16\n"
        "s_load_dword rbx, rbx5[2:3], 0x5b\n"
        "s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b\n"
        "s_load_dwordx4 rax4[0:3], rbx5[6:7], 0x5b\n"
        "s_load_dwordx8 rax4[0:7], rbx5[8:9], 0x5b\n"
        "s_load_dwordx16 rax4[4:19], rbx5[10:11], 0x5b\n"
        "s_load_dword rbx, rbx5[2:3], rbx5[6]\n"
        "s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b\n"
        "s_memtime  rax4[2:3]\n"
        "s_dcache_inv\n"
        "s_store_dword rbx, rbx5[2:3], 0x5b\n"
        "s_atc_probe  0x32, rax4[12:13], 0xfff5b\n"
        "s_atc_probe_buffer  0x32, rax4[12:15], 0xfff5b\n"
        "s_load_dwordx2 s[28:29], s[36:37], 0x5b\n"
        "s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b\n",
        {
            // s_load_dword rbx, rbx5[2:3], 0x5b
            { 0, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b
            { 8, "rax4", 0, 2, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rbx5", 4, 6, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx4 rax4[0:3], rbx5[4:5], 0x5b
            { 16, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 16, "rbx5", 6, 8, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx8 rax4[0:7], rbx5[4:5], 0x5b
            { 24, "rax4", 0, 8, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 24, "rbx5", 8, 10, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx16 rax4[4:19], rbx5[4:5], 0x5b
            { 32, "rax4", 4, 20, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 32, "rbx5", 10, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dword rbx, rbx5[2:3], rbx5[6]
            { 40, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 40, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            { 40, "rbx5", 6, 7, GCNFIELD_SMRD_SOFFSET, ASMRVU_READ, 1 },
            // s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b
            { 48, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 48, "rbx5", 8, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_memtime  rax4[2:3]
            { 56, "rax4", 2, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            // s_store_dword rbx, rbx5[2:3], 0x5b\n
            { 72, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_READ, 1 },
            { 72, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_atc_probe  0x32, rax4[12:13], 0xfff5b
            { 80, "rax4", 12, 14, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_atc_probe_buffer 0x32, rax4[12:13], 0xfff5b
            { 88, "rax4", 12, 16, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_load_dwordx2 s[28:29], s[36:37], 0x5b
            { 96, nullptr, 28, 30, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 96, nullptr, 36, 38, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 },
            // s_buffer_load_dwordx4 s[44:47], s[12:15], 0x5b
            { 104, nullptr, 44, 48, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 0 },
            { 104, nullptr, 12, 16, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* VOP2 */
        ".regvar rax:v, rbx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_sub_f32  rex, rax, rbx\n"
        "v_sub_f32  rex, srex, rbx\n"
        "v_cndmask_b32 rex, rax, rbx, vcc\n"
        "v_addc_u32  rex, vcc, rax, rbx, vcc\n"
        "v_readlane_b32 srex, rax2[3], srdx3[4]\n"
        "v_writelane_b32 rax, rax2[4], srdx3[3]\n"
        "v_sub_f32  rex, rax, rbx vop3\n"
        "v_readlane_b32 srex, rax2[3], srdx3[4] vop3\n"
        "v_addc_u32  rex, srdx3[0:1], rax, rbx, srdx3[2:3]\n"
        "v_sub_f32  rex, rax, srbx\n"
        "v_sub_f32  v46, v42, v22\n"
        "v_sub_f32  v46, s42, v22\n"
        "v_addc_u32  v17, vcc, v53, v25, vcc\n"
        "v_readlane_b32 s45, v37, s14\n"
        "v_addc_u32  v67, s[4:5], v58, v13, s[18:19]\n"
        "v_readlane_b32 s51, v26, s37 vop3\n",
        {
            // v_sub_f32  rex, rax, rbx
            { 0, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_sub_f32  rex, srex, rbx
            { 4, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 4, "srex", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 4, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cndmask_b32 rex, rax, rbx, vcc
            { 8, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 8, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 8, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_addc_u32  rex, vcc, rax, rbx, vcc
            { 12, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 12, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 12, "rbx", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_readlane_b32 srex, rax2[3], srdx3[4]
            { 16, "srex", 0, 1, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 1 },
            { 16, "rax2", 3, 4, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 16, "srdx3", 4, 5, GCNFIELD_VOP_SSRC1, ASMRVU_READ, 1 },
            // v_writelane_b32 rax, rax2[4], srdx3[3]
            { 20, "rax", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 20, "rax2", 4, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 20, "srdx3", 3, 4, GCNFIELD_VOP_SSRC1, ASMRVU_READ, 1 },
            /* vop3 encoding */
            // v_sub_f32  rex, rax, rbx vop3
            { 24, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 24, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 24, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_readlane_b32 srex, rax2[3], srdx3[4] vop3
            { 32, "srex", 0, 1, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 1 },
            { 32, "rax2", 3, 4, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 32, "srdx3", 4, 5, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_addc_u32  rex, srex, rax, rbx, srdx3[1]
            { 40, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 40, "srdx3", 0, 2, GCNFIELD_VOP3_SDST1, ASMRVU_WRITE, 1 },
            { 40, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 40, "rbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            { 40, "srdx3", 2, 4, GCNFIELD_VOP3_SSRC, ASMRVU_READ, 1 },
            // v_sub_f32  rex, rax, srbx
            { 48, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 48, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 48, "srbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_sub_f32  v46, v42, v22
            { 56, nullptr, 256+46, 256+47, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 56, nullptr, 256+42, 256+43, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 56, nullptr, 256+22, 256+23, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_sub_f32  v46, s42, v22
            { 60, nullptr, 256+46, 256+47, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 60, nullptr, 42, 43, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 60, nullptr, 256+22, 256+23, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_addc_u32  v17, vcc, v53, v25, vcc
            { 64, nullptr, 256+17, 256+18, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 64, nullptr, 256+53, 256+54, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 64, nullptr, 256+25, 256+26, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_readlane_b32 s45, v37, s14
            { 68, nullptr, 45, 46, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 0 },
            { 68, nullptr, 256+37, 256+38, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 68, nullptr, 14, 15, GCNFIELD_VOP_SSRC1, ASMRVU_READ, 0 },
            // v_addc_u32  v67, s[4:5], v58, v13, s[18:19]
            { 72, nullptr, 256+67, 256+68, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 72, nullptr, 4, 6, GCNFIELD_VOP3_SDST1, ASMRVU_WRITE, 0 },
            { 72, nullptr, 256+58, 256+59, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 72, nullptr, 256+13, 256+14, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            { 72, nullptr, 18, 20, GCNFIELD_VOP3_SSRC, ASMRVU_READ, 0 },
            // v_readlane_b32 s51, v26, s37 vop3
            { 80, nullptr, 51, 52, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 0 },
            { 80, nullptr, 256+26, 256+27, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 80, nullptr, 37, 38, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 }
        },
        true, ""
    },
    {   /* VOP1 */
        ".regvar rax:v, rbx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_cvt_f32_i32 rex, rax\n"
        "v_cvt_f32_i32 rex, srbx\n"
        "v_rcp_f64 rax2[2:3], rbx4[5:6]\n"
        "v_rcp_f64 rax2[2:3], srdx3[1:2]\n"
        "v_readfirstlane_b32 srex, rbx\n"
        "v_nop\n"
        "v_cvt_i32_f64 rbx, rax2[3:4]\n"
        "v_cvt_f32_i32 rex, rax vop3\n"
        "v_cvt_f32_i32 rex, srbx vop3\n"
        "v_rcp_f64 rax2[2:3], rbx4[5:6] vop3\n"
        "v_rcp_f64 rax2[2:3], srdx3[1:2] vop3\n"
        "v_readfirstlane_b32 srex, rbx vop3\n"
        "v_cvt_f32_i32 v43, v147\n"
        "v_cvt_f32_i32 v51, s19\n"
        "v_rcp_f64 v[72:73], v[27:28]\n"
        "v_rcp_f64 v[72:73], s[27:28]\n"
        "v_readfirstlane_b32 s35, v91\n"
        "v_rcp_f64 v[55:56], v[87:88] vop3\n"
        "v_cvt_f32_i32 v43, v147 vop3\n",
        {
            // v_cvt_f32_i32 rex, rax
            { 0, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 rex, srbx
            { 4, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 4, "srbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], rbx4[6:7]
            { 8, "rax2", 2, 4, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 8, "rbx4", 5, 7, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], srdx3[1:2]
            { 12, "rax2", 2, 4, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 12, "srdx3", 1, 3, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_readfirstlane_b32 srex, rbx
            { 16, "srex", 0, 1, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 1 },
            { 16, "rbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_cvt_i32_f64 rbx, rax2[3:4]
            { 24, "rbx", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 24, "rax2", 3, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 rex, rax vop3
            { 28, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 28, "rax", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 rex, srbx vop3
            { 36, "rex", 0, 1, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 36, "srbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], rbx4[6:7] vop3
            { 44, "rax2", 2, 4, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 44, "rbx4", 5, 7, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_rcp_f64 rax2[2:3], srdx3[1:2] vop3
            { 52, "rax2", 2, 4, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 1 },
            { 52, "srdx3", 1, 3, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_readfirstlane_b32 srex, rbx vop3
            { 60, "srex", 0, 1, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 1 },
            { 60, "rbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            // v_cvt_f32_i32 v43, v147
            { 68, nullptr, 256+43, 256+44, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 68, nullptr, 256+147, 256+148, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_cvt_f32_i32 v51, s19
            { 72, nullptr, 256+51, 256+52, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 72, nullptr, 19, 20, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_rcp_f64 v[72:73], v[27:28]
            { 76, nullptr, 256+72, 256+74, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 76, nullptr, 256+27, 256+29, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_rcp_f64 v[72:73], s[27:28]
            { 80, nullptr, 256+72, 256+74, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 0 },
            { 80, nullptr, 27, 29, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_readfirstlane_b32 s35, v91
            { 84, nullptr, 35, 36, GCNFIELD_VOP_SDST, ASMRVU_WRITE, 0 },
            { 84, nullptr, 256+91, 256+92, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            // v_rcp_f64 v[55:56], v[87:88] vop3
            { 88, nullptr, 256+55, 256+57, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 88, nullptr, 256+87, 256+89, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            // v_cvt_f32_i32 v43, v147
            { 96, nullptr, 256+43, 256+44, GCNFIELD_VOP3_VDST, ASMRVU_WRITE, 0 },
            { 96, nullptr, 256+147, 256+148, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
        },
        true, ""
    },
    {   /* VOPC */
        ".regvar rax:v, rbx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6, srbx:s\n"
        "v_cmp_gt_u32 vcc, rbx, rex\n"
        "v_cmp_gt_u64 vcc, rax2[3:4], rbx4[6:7]\n"
        "v_cmp_gt_u32 vcc, srbx, rex\n"
        "v_cmp_gt_u32 srdx3[2:3], rbx, rex\n"
        "v_cmp_gt_u32 vcc, rbx, srbx\n"
        "v_cmp_gt_u64 vcc, srdx3[3:4], rbx4[6:7]\n"
        "v_cmp_gt_u32 vcc, v72, v41\n"
        "v_cmp_gt_u64 vcc, v[65:66], v[29:30]\n"
        "v_cmp_gt_u64 s[46:47], v[65:66], v[29:30]\n"
        "v_cmp_gt_u32 vcc, v72, s41\n",
        {
            // v_cmp_gt_u32 vcc, rbx, rex
            { 0, "rbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 0, "rex", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u64 vcc, rax[3:4], rbx4[6:7]
            { 4, "rax2", 3, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 4, "rbx4", 6, 8, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 vcc, rbx, rex
            { 8, "srbx", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 8, "rex", 0, 1, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 srdx3[2:3], rbx, rex
            { 12, "srdx3", 2, 4, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 1 },
            { 12, "rbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 12, "rex", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 vcc, rbx, srbx
            { 20, "rbx", 0, 1, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 1 },
            { 20, "srbx", 0, 1, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u64 vcc, srdx3[3:4], rbx4[6:7]
            { 28, "srdx3", 3, 5, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 28, "rbx4", 6, 8, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 1 },
            // v_cmp_gt_u32 vcc, v72, v41
            { 32, nullptr, 256+72, 256+73, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 32, nullptr, 256+41, 256+42, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_cmp_gt_u64 vcc, v[65:66], v[29:30]
            { 36, nullptr, 256+65, 256+67, GCNFIELD_VOP_SRC0, ASMRVU_READ, 0 },
            { 36, nullptr, 256+29, 256+31, GCNFIELD_VOP_VSRC1, ASMRVU_READ, 0 },
            // v_cmp_gt_u64 s[46:47], v[65:66], v[29:30]
            { 40, nullptr, 46, 48, GCNFIELD_VOP3_SDST0, ASMRVU_WRITE, 0 },
            { 40, nullptr, 256+65, 256+67, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 40, nullptr, 256+29, 256+31, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
            // v_cmp_gt_u32 vcc, v72, s41
            { 48, nullptr, 256+72, 256+73, GCNFIELD_VOP3_SRC0, ASMRVU_READ, 0 },
            { 48, nullptr, 41, 42, GCNFIELD_VOP3_SRC1, ASMRVU_READ, 0 },
        },
        true, ""
    }
};

static void testGCNRegVarUsages(cxuint i, const GCNRegVarUsageCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << " regVarUsageGCNCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testGCNRegVarUsages", testCaseName+".good",
                      testCase.good, good);
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " regVarUsageGCNCase#" << i;
        throw Exception(oss.str());
    }
    /*assertValue("testGCNRegVarUsages", testCaseName+".size",
                    testCase.regVarUsages.size(), section.regVarUsages.size());*/
    ISAUsageHandler* usageHandler = assembler.getSections()[0].usageHandler.get();
    usageHandler->rewind();
    for (size_t j = 0; usageHandler->hasNext(); j++)
    {
        const AsmRegVarUsage resultRvu = usageHandler->nextUsage();
        std::ostringstream rvuOss;
        rvuOss << ".regVarUsage#" << j << ".";
        rvuOss.flush();
        std::string rvuName(rvuOss.str());
        const AsmRegVarUsageData& expectedRvu = testCase.regVarUsages[j];
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"offset",
                    expectedRvu.offset, resultRvu.offset);
        if (expectedRvu.regVarName==nullptr)
            assertTrue("testGCNRegVarUsages", testCaseName+rvuName+"regVarName",
                       resultRvu.regVar==nullptr);
        else // otherwise
            assertString("testGCNRegVarUsages", testCaseName+rvuName+"regVarName",
                        expectedRvu.regVarName, resultRvu.regVar->first);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rstart",
                    expectedRvu.rstart, resultRvu.rstart);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rend",
                    expectedRvu.rend, resultRvu.rend);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"regField",
                    cxuint(expectedRvu.regField), cxuint(resultRvu.regField));
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rwFlags",
                    cxuint(expectedRvu.rwFlags), cxuint(resultRvu.rwFlags));
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"align",
                    cxuint(expectedRvu.align), cxuint(resultRvu.align));
    }
    assertString("testGCNRegVarUsages", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(gcnRvuTestCases1Tbl)/sizeof(GCNRegVarUsageCase); i++)
        try
        { testGCNRegVarUsages(i, gcnRvuTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}