/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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

#ifndef __CLRX_GCNASMINTERNALS_H__
#define __CLRX_GCNASMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"
#include "GCNInternals.h"

namespace CLRX
{

enum : Flags {
    INSTROP_SREGS = 1,
    INSTROP_SSOURCE = 2,
    INSTROP_VREGS = 4,
    INSTROP_LDS = 8,
    INSTROP_VOP3MODS = 0x10,
    // used internally by parseGCNOperand to continuing parsing with modifiers
    INSTROP_PARSEWITHNEG = 0x20,
    INSTROP_VOP3NEG = 0x40,
    
    INSTROP_ONLYINLINECONSTS = 0x80, /// accepts only inline constants
    INSTROP_NOLITERALERROR = 0x100, /// accepts only inline constants
    INSTROP_NOLITERALERRORMUBUF = 0x200, /// accepts only inline constants
    
    INSTROP_TYPE_MASK = 0x3000,
    INSTROP_INT = 0x000,    // integer literal
    INSTROP_FLOAT = 0x1000, // floating point literal
    INSTROP_F16 = 0x2000,   // half floating point literal
};

enum: cxbyte {
    VOPOP_ABS = 1,
    VOPOP_NEG = 2,
    VOPOP_SEXT = 4
};

enum: cxbyte {
    VOP3_MUL2 = 1,
    VOP3_MUL4 = 2,
    VOP3_DIV2 = 3,
    VOP3_CLAMP = 16,
    VOP3_VOP3 = 32,
    VOP3_BOUNDCTRL = 64
};

struct RegRange
{
    uint16_t start, end;
    
    RegRange(): start(0), end(0) { }
    template<typename U1, typename U2>
    RegRange(U1 t1, U2 t2 = 0) : start(t1), end(t2) { }
    
    bool operator!() const
    { return start==0 && end==0; }
    operator bool() const
    { return start!=0 || end!=0; }
};

struct GCNOperand {
    RegRange range;
    uint32_t value;
    cxbyte vopMods;
    
    bool operator!() const
    { return !range; }
    operator bool() const
    { return range; }
};

struct VOPExtraModifiers
{
    cxuint dstSel:3;
    cxuint dstUnused:2;
    cxuint src0Sel:3;
    cxuint src1Sel:3;
    cxuint bankMask:4;
    cxuint rowMask:4;
    cxuint dppCtrl:9;
    cxuint needSDWA:1;
    cxuint needDPP:1;
};

struct CLRX_INTERNAL GCNAsmUtils: AsmParseUtils
{
    static bool printRegisterRangeExpected(Assembler& asmr, const char* linePtr,
               const char* regPoolName, cxuint regsNum, bool required);
    
    static void printXRegistersRequired(Assembler& asmr, const char* linePtr,
               const char* regPoolName, cxuint requiredRegsNum);
    
    /* return true if no error */
    static bool parseVRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                   cxuint regsNum, bool required = true);
    /* return true if no error */
    static bool parseSRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                   uint16_t arch, cxuint regsNum, bool required = true);
    
    /* return true if no error */
    static bool parseImmInt(Assembler& asmr, const char*& linePtr, uint32_t& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, cxuint bits = 0,
            cxbyte signess = WS_BOTH);
    
    template<typename T>
    static bool parseImm(Assembler& asmr, const char*& linePtr, T& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, cxuint bits = 0,
            cxbyte signess = WS_BOTH)
    {
        uint32_t v32;
        bool ret = parseImmInt(asmr, linePtr, v32, outTargetExpr,
               (bits!=0?bits:(sizeof(T)<<3)), signess);
        if ((outTargetExpr==nullptr || *outTargetExpr==nullptr) && ret)
            value = v32;
        return ret;
    }
    
    static bool parseLiteralImm(Assembler& asmr, const char*& linePtr, uint32_t& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, Flags instropMask = 0);
    
    /* withSDWAOperands - number operand that will be handled by SDWA modifer parser,
     * (includes destination at begin) */
    static bool parseVOPModifiers(Assembler& asmr, const char*& linePtr, cxbyte& mods,
                       VOPExtraModifiers* extraMods = nullptr,
                       bool withClamp = true, cxuint withSDWAOperands = 3);
    
    static bool parseOperand(Assembler& asmr, const char*& linePtr, GCNOperand& operand,
               std::unique_ptr<AsmExpression>* outTargetExpr, uint16_t arch,
               cxuint regsNum, Flags instrOpMask);
    
    template<typename T>
    static bool parseModImm(Assembler& asmr, const char*& linePtr, T& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, const char* modName,
            cxuint bits = 0, cxbyte signess = WS_BOTH)
    {
        if (outTargetExpr!=nullptr)
            outTargetExpr->reset();
        const char* end = asmr.line+asmr.lineSize;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr==':')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            return parseImm(asmr, linePtr, value, outTargetExpr, bits, signess);
        }
        asmr.printError(linePtr, (std::string("Expected ':' before ")+modName).c_str());
        return false;
    }
    
    static bool parseVINTRP0P10P20(Assembler& asmr, const char*& linePtr, RegRange& reg);
    static bool parseVINTRPAttr(Assembler& asmr, const char*& linePtr, cxbyte& attr);
    
    static bool getMUBUFFmtNameArg(Assembler& asmr, size_t maxOutStrSize, char* outStr,
               const char*& linePtr, const char* objName);
    
    static void parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOPKEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOPPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    
    static void parseSMRDEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSMEMEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    
    static void parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseMUBUFEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
};

static inline bool isXRegRange(RegRange pair, cxuint regsNum = 1)
{   // second==0 - we assume that first is inline constant, otherwise we check range
    return (pair.end==0) || cxuint(pair.end-pair.start)==regsNum;
}

static inline void updateVGPRsNum(cxuint& vgprsNum, cxuint vgpr)
{
    vgprsNum = std::min(std::max(vgprsNum, vgpr+1), 256U);
}

static inline void updateSGPRsNum(cxuint& sgprsNum, cxuint sgpr, uint16_t arch)
{
    cxuint maxSGPRsNum = arch&ARCH_RX3X0 ? 102U: 104U;
    if (sgpr < maxSGPRsNum) /* sgpr=-1 does not change sgprsNum */
        sgprsNum = std::min(std::max(sgprsNum, sgpr+1), arch&ARCH_RX3X0 ? 100U: 102U);
}

};

#endif