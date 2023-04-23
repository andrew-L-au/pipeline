#ifndef PIPELINE_SIM_H
#define PIPELINE_SIM_H

#include <iostream>
#include <bits/stdc++.h>
#include <string>

#include "arm_instr.h"

using namespace std;

struct IF_ID {
    int prog_cnt = 0;
    Instruction recent_instr;
};

struct ID_EX {
    int dest = 0;
    int r1 = 0;
    int r2 = 0;
    int r1Data = 0;
    int r2Data = 0;
    int immidiate = 0;
    int address = 0;
    int offset = 0;
    int prog_cnt = 0;
    bool load_use_hazard = false;
    ARM_OPC opcode = OPC_INVALID;
    ARM_INSTR_TYPE type = INSTR_TYPE_INVALID;
};

struct EX_MEM {
    int val_arith = 0;
    int val_address = 0;
    int dest = 0;
    bool zero = 0;
    int r1Data = 0;
    int r2Data = 0;
    int prog_cnt = 0;
    bool load_use_hazard = false;
    ARM_OPC opcode = OPC_INVALID;
    ARM_INSTR_TYPE type = INSTR_TYPE_INVALID;
};

struct MEM_WB {
    int val_data = 0;
    int dest = 0;
    int prog_cnt = 0;
    bool load_use_hazard = false;
    ARM_OPC opcode = OPC_INVALID;
    ARM_INSTR_TYPE type = INSTR_TYPE_INVALID;
};

#endif // PIPELINE_SIM_H
