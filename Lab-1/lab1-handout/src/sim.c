#include <stdio.h>
#include "shell.h"

/********************************************************************/
/* part: the input
 * num: the times to repeat bit and emplace it ahead
 * bit: the position of bit to move */
/********************************************************************/
uint32_t sign_extended(uint32_t part, int num, int bit) {
    uint32_t result = 0;
    for (int i = 31; i >= 31-num+1; i--)
        result += (part >> (bit-1)) << i;
    return result;
}

void process_instruction()
{
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC); // read instruction from memory
    uint8_t op = (instruction >> 26) & 0xFF; // compute the 6-bit operation code

    /***********************************************************/
    /****** Process the situation of J type instruction ********/
    /***********************************************************/
    if (2 <= op && op <= 3) {
        uint32_t target = (instruction << 6) / 64;
        uint32_t temp = target;
        if (op == 3)
            NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 8; // Jump And Link
        NEXT_STATE.PC = (((CURRENT_STATE.PC >> 27) & 0xFF) << 27) + (temp << 2);
        return;
    }

    /***********************************************************/
    /****** Process the situation of I type instruction ********/
    /***********************************************************/
    if (op >= 4 || op == 1) {
        uint32_t p1 = (instruction << 6) >> 27;
        uint32_t p2 = (instruction << 11) >> 27;
        uint32_t p3 = (instruction << 16) >> 16;

        /*Process branch instructions
         * In these cases the p3 is offset*/
        if ((op >= 4 && op <= 7) || op == 1) {
            /*In these cases p1 is rs, p2 is rt*/
            uint32_t target = 0;
            target = sign_extended(p3, 14, 16);
            target += p3 << 2;
            int condition = 0; // use a int variable to represent the bool

            /*determine the condition according to instruction*/
            switch (op) {
                case 4:
                    condition = CURRENT_STATE.REGS[p1] == CURRENT_STATE.REGS[p2]; // BEQ
                    break;
                case 5:
                    condition = CURRENT_STATE.REGS[p1] != CURRENT_STATE.REGS[p2]; // BNE
                    break;
                case 6:
                    condition = (CURRENT_STATE.REGS[p1] >> 31) == 1 || CURRENT_STATE.REGS[p1] == 0; // BLEZ
                    break;
                case 7:
                    condition = (CURRENT_STATE.REGS[p1] >> 31) == 0 && CURRENT_STATE.REGS[p1] != 0; // BGTZ
                    break;
                case 1:
                    /*These cases instruction type is determined by p2*/
                    switch (p2) {
                        case 0:
                            condition = CURRENT_STATE.REGS[p1] >> 31 == 1; // BLTZ
                            break;
                        case 1:
                            condition = CURRENT_STATE.REGS[p1] >> 31 == 0; // BGEZ
                            break;
                        case 16:
                            condition = CURRENT_STATE.REGS[p1] >> 31 == 1; // BLTZAL
                            NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 8;
                            break;
                        case 17:
                            condition = CURRENT_STATE.REGS[p1] >> 31 == 0; // BGEZAL
                            NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 8;
                            break;
                        default:
                            condition = 0;
                    }
                    break;
                default:
                    condition = 0;
            }

            /*if condition is true, move to target instruction*/
            if (condition) {
                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                return;
            }

            /*if condition is false, move to next instruction*/
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            return;
        }

        /*Process immediate operations*/
        if (op >= 8 && op <= 15) {
            /*In these cases p1 is rs, p2 is rt*/
            uint32_t temp = sign_extended(p3, 16, 16) + p3;
            switch (op) {
                case 8:
                case 9:
                    NEXT_STATE.REGS[p2] = CURRENT_STATE.REGS[p1] + temp; // ADDI or ADDIU
                    break;
                case 10:
                case 11:
                    if (CURRENT_STATE.REGS[p1] < temp) // SLTI or SLTIU ??? difference???
                        NEXT_STATE.REGS[p2] = 1;
                    else
                        NEXT_STATE.REGS[p2] = 0;
                    break;
                case 12:
                    NEXT_STATE.REGS[p2] = p3 & (CURRENT_STATE.REGS[p1] << 16) >> 16; // ANDI
                    break;
                case 13:
                    NEXT_STATE.REGS[p2] = p3 | CURRENT_STATE.REGS[p1]; // ORI
                    break;
                case 14:
                    NEXT_STATE.REGS[p2] = p3 ^ CURRENT_STATE.REGS[p1]; // XORI
                    break;
                case 15:
                    NEXT_STATE.REGS[p2] = p3 << 16; // LUI
                    break;
                default:
                    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
                    return;
            }
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            return;
        }

        /* Process Load and store Instructions */
        if (op >= 32) {
            uint32_t address = sign_extended(p3, 16, 16) + p3 + CURRENT_STATE.REGS[p1];
            uint32_t data = 0;
            switch (op) {
                case 32:
                    // Load Byte (LB)
                    data = mem_read_32(address) & 0xFF;
                    CURRENT_STATE.REGS[p2] = sign_extended(data, 24, 8) + data;
                    break;
                case 33:
                    // Load Halfword (LH)
                    data = mem_read_32(address) & 0xFFFF;
                    CURRENT_STATE.REGS[p2] = sign_extended(data, 16, 16) + data;
                    break;
                case 35:
                    // Load Word (LW)
                    data = mem_read_32(address);
                    CURRENT_STATE.REGS[p2] = data;
                    break;
                case 36:
                    // Load Byte Unsigned (LBU)
                    CURRENT_STATE.REGS[p2] = mem_read_32(address) & 0xFF;
                    break;
                case 37:
                    // Load Halfword Unsigned (LHU)
                    CURRENT_STATE.REGS[p2] = mem_read_32(address) & 0xFFFF;
                    break;
                case 40:
                    // Store Byte
                    mem_write_32(address, (int)(CURRENT_STATE.REGS[p2]) & 0xFF);
                    break;
                case 41:
                    mem_write_32(address, (int)(CURRENT_STATE.REGS[p2]) & 0xFFFF);
                    break;
                case 43:
                    mem_write_32(address, (int)(CURRENT_STATE.REGS[p2]));
                    break;
                default:
                    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
                    return;
            }
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            return;
        }
    }

    /***********************************************************/
    /****** Process the situation of R type instruction ********/
    /***********************************************************/
    if (op == 0) {
        uint32_t p1 = (instruction << 6) >> 27;
        uint32_t p2 = (instruction << 11) >> 27;
        uint32_t p3 = (instruction << 16) >> 27;
        uint32_t p4 = (instruction << 21) >> 27;
        uint32_t p5 = (instruction << 26) >> 26; // indicate the instruction type

        uint64_t t = 0; // temp value for MULT

        /*In these cases p2 is rt, p3 is rd, p4 is sa*/
        switch (p5) {
            case 0:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p2] << p4; // SLL
                break;
            case 2:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p2] >> p4; // SRL
                break;
            case 3:
                // SRA
                NEXT_STATE.REGS[p3] = sign_extended(CURRENT_STATE.REGS[p2], p4, 32) + (CURRENT_STATE.REGS[p2] >> p4);
                break;
            case 4:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p2] << ((CURRENT_STATE.REGS[p1] << 27) >> 27); // SLLV
                break;
            case 5:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p2] >> ((CURRENT_STATE.REGS[p1] << 27) >> 27); // SRLV
                break;
            case 6:
                NEXT_STATE.REGS[p3] = (CURRENT_STATE.REGS[p2] >> ((CURRENT_STATE.REGS[p1] << 27) >> 27)) +
                        sign_extended(CURRENT_STATE.REGS[p2], (CURRENT_STATE.REGS[p1] << 27) >> 27, 32); // SRAV
                break;
            case 8:
                NEXT_STATE.PC = CURRENT_STATE.REGS[p1]; // JR
                return;
            case 9:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.PC + 8;
                NEXT_STATE.PC = CURRENT_STATE.REGS[p1]; // JALR
                return;
            case 32:
            case 33:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p1] + CURRENT_STATE.REGS[p2]; // ADD & ADDU
                break;
            case 34:
            case 35:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p1] - CURRENT_STATE.REGS[p2]; // SUB & SUBU
                break;
            case 36:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p1] & CURRENT_STATE.REGS[p2]; // AND
                break;
            case 37:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p1] | CURRENT_STATE.REGS[p2]; // OR
                break;
            case 38:
                NEXT_STATE.REGS[p3] = CURRENT_STATE.REGS[p1] ^ CURRENT_STATE.REGS[p2]; // XOR
                break;
            case 39:
                NEXT_STATE.REGS[p3] = ~(CURRENT_STATE.REGS[p1] | CURRENT_STATE.REGS[p2]); // NOR
                break;
            case 42:
                if (CURRENT_STATE.REGS[p1] < CURRENT_STATE.REGS[p2]) // SLT
                    NEXT_STATE.REGS[p3] = 1;
                else
                    NEXT_STATE.REGS[p3] = 0;
            case 43:
                if (CURRENT_STATE.REGS[p1] >> 1 < CURRENT_STATE.REGS[p2] >> 1) // SLTU
                    NEXT_STATE.REGS[p3] = 1;
                else
                    NEXT_STATE.REGS[p3] = 0;
            case 24:
                // MULT
                t = CURRENT_STATE.REGS[p1] * CURRENT_STATE.REGS[p2];
                NEXT_STATE.LO = (t << 32) >> 32;
                NEXT_STATE.HI = t >> 32;
                break;
            case 16:
                // MFHI
                NEXT_STATE.REGS[p3] = CURRENT_STATE.HI;
                break;
            case 18:
                // MFLO
                NEXT_STATE.REGS[p3] = CURRENT_STATE.LO;
                break;
            case 17:
                // MTHI
                NEXT_STATE.HI = CURRENT_STATE.REGS[p1];
                break;
            case 19:
                // MTLO
                NEXT_STATE.LO = CURRENT_STATE.REGS[p1];
                break;
            case 25:
                // MULTU
                t = CURRENT_STATE.REGS[p1] >> 1 * CURRENT_STATE.REGS[p2] >> 1;
                NEXT_STATE.LO = (t << 32) >> 32;
                NEXT_STATE.HI = t >> 32;
                break;
            case 20:
                // DIV
                NEXT_STATE.LO = CURRENT_STATE.REGS[p1] / CURRENT_STATE.REGS[p2];
                NEXT_STATE.HI = CURRENT_STATE.REGS[p1] % CURRENT_STATE.REGS[p2];
                break;
            case 21:
                // DIVU
                NEXT_STATE.LO = CURRENT_STATE.REGS[p1] >> 1 / CURRENT_STATE.REGS[p2] >> 1;
                NEXT_STATE.HI = CURRENT_STATE.REGS[p1] >> 1 % CURRENT_STATE.REGS[p2] >> 1;
                break;
            default:
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
                return;
        }
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
        return;
    }
}
