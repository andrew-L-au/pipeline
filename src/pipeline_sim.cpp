#include "pipeline_sim.h"
#include "parser.h"

/**
 * For simplicity, we only use the 16 registers in ARM 32.
 * Memory is 4000 bytes, and top 1000 bytes are preserved.
 * Stack pointer (reg[ARM_REG_SP]) is initialized to 3000.
*/
int reg[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3000, 0, 0};
int mem[4000 / sizeof(int)];

vector<Instruction> instructions;
vector<int> latencies(12, 0);

bool load_use_hazard = false;
bool control_hazard = false;
bool shut_down = false;
bool file_end = false;

ofstream fout;


// ====================== PIPELINE STAGES ======================

IF_ID Register_IF_ID;
ID_EX Register_ID_EX;
EX_MEM Register_EX_MEM;
MEM_WB Register_MEM_WB;

/**
 * Fetch
*/
void IF() {

    /*************************************/
    /* TODO: Fix and complete IF stage.  */
    /* Hint: if hazards happen, you may  */
    /*  need to change some operations.  */
    /*************************************/

    if ((reg[ARM_REG_PC] / 4) >= instructions.size()) {
        Register_IF_ID.prog_cnt = reg[ARM_REG_PC] / 4;
        Instruction end_file;
        end_file.opcode = END_OF_FILE;
        Register_IF_ID.recent_instr = end_file;
    } else {
        Register_IF_ID.prog_cnt = reg[ARM_REG_PC] / 4;
        Register_IF_ID.recent_instr = instructions[reg[ARM_REG_PC] / 4];
        reg[ARM_REG_PC] += 4;
    }
    fout << ARM_OPC_NAME[Register_IF_ID.recent_instr.opcode] << endl;
}

/**
 * Decode
*/
void ID() {

    /*************************************/
    /* TODO: Fix and complete ID stage.  */
    /* Note: you are expected to detect  */
    /*  load-use data hazard and support */
    /*  forwarding in this stage.        */
    /*************************************/
    Register_ID_EX.dest = 0;
    Register_ID_EX.r1 = 0;
    Register_ID_EX.r2 = 0;
    Register_ID_EX.prog_cnt = Register_IF_ID.prog_cnt;
    Register_ID_EX.load_use_hazard = false;
    Register_ID_EX.opcode = Register_IF_ID.recent_instr.opcode;
    Register_ID_EX.type = Register_IF_ID.recent_instr.type;
    ARM_OPC preInstruct_OPC = Register_ID_EX.opcode;
    int preInstructDest = Register_ID_EX.dest;
    bool preInstructIsLoad = preInstruct_OPC == OPC_LDR;
    //detect load-use data hazard
    load_use_hazard = false;

    if (Register_IF_ID.recent_instr.opcode == OPC_MOV){
        if (Register_IF_ID.recent_instr.type == INSTR_TYPE_REG){
            Register_ID_EX.dest = Register_IF_ID.recent_instr.dest;
            Register_ID_EX.r2 = Register_IF_ID.recent_instr.operand1;
            Register_ID_EX.r2Data = reg[Register_ID_EX.r2]; //don't need r1
            if (preInstructIsLoad && preInstructDest == Register_ID_EX.r2){
                load_use_hazard = true;
            }
        }else if (Register_IF_ID.recent_instr.type == INSTR_TYPE_IMM){
            Register_ID_EX.dest = Register_IF_ID.recent_instr.dest; //don't need r2 and r2
            Register_ID_EX.immidiate = Register_IF_ID.recent_instr.operand1; 
        }else if (Register_IF_ID.recent_instr.type == INSTR_TYPE_EXTRA){ // pc lr
            Register_ID_EX.dest = ARM_REG_PC;
            Register_ID_EX.r2 = ARM_REG_LR;
            Register_ID_EX.r2Data = reg[Register_ID_EX.r2]; //don't need r1
            if (preInstructIsLoad && preInstructDest == Register_ID_EX.r2){
                load_use_hazard = true;
            }
        }
    }else if (Register_IF_ID.recent_instr.opcode == OPC_STR){
        Register_ID_EX.r1 = Register_IF_ID.recent_instr.operand1;
        Register_ID_EX.r2 = Register_IF_ID.recent_instr.dest;
        Register_ID_EX.r1Data = reg[Register_ID_EX.r1];
        Register_ID_EX.r2Data = reg[Register_ID_EX.r2];
        Register_ID_EX.immidiate = Register_IF_ID.recent_instr.operand2;
        if (preInstructIsLoad && (preInstructDest == Register_ID_EX.r1 || preInstructDest == Register_ID_EX.r2)){
            load_use_hazard = true;
        }
    }
    else if (Register_IF_ID.recent_instr.opcode == OPC_B || Register_IF_ID.recent_instr.opcode == OPC_BL){
        Register_ID_EX.dest = Register_IF_ID.recent_instr.dest;
        //no data
    }else if (Register_IF_ID.recent_instr.type == INSTR_TYPE_REG) {
        Register_ID_EX.dest = Register_IF_ID.recent_instr.dest;
        Register_ID_EX.r1 = Register_IF_ID.recent_instr.operand1;
        Register_ID_EX.r2 = Register_IF_ID.recent_instr.operand2;
        Register_ID_EX.r1Data = reg[Register_ID_EX.r1];
        Register_ID_EX.r2Data = reg[Register_ID_EX.r2];
        if (preInstructIsLoad && (preInstructDest == Register_ID_EX.r1 || preInstructDest == Register_ID_EX.r2)){
            load_use_hazard = true;
        }
    }else if (Register_IF_ID.recent_instr.type == INSTR_TYPE_IMM) {
        Register_ID_EX.dest = Register_IF_ID.recent_instr.dest;
        Register_ID_EX.r1 = Register_IF_ID.recent_instr.operand1;
        Register_ID_EX.r1Data = reg[Register_ID_EX.r1];
        Register_ID_EX.immidiate = Register_IF_ID.recent_instr.operand2;
        if (preInstructIsLoad && preInstructDest == Register_ID_EX.r1){
            load_use_hazard = true;
        }
    }

    Register_ID_EX.load_use_hazard = load_use_hazard;
    fout << ARM_OPC_NAME[Register_ID_EX.opcode] << endl;
}

/**
 * Execute
*/
bool EX_MEM_RegWrite = false;
bool MEM_WB_RegWrite = false;
int MEM_WB_Result;
int MEM_WB_Dest;
int EX_MEM_Result;
int EX_MEM_Dest;

void EX() {

    /*************************************/
    /* TODO: Fix and complete EX stage.  */
    /* Hint: finish computing.           */
    /*************************************/

    Register_EX_MEM.dest = Register_ID_EX.dest;
    Register_EX_MEM.load_use_hazard = Register_ID_EX.load_use_hazard;
    Register_EX_MEM.prog_cnt = Register_ID_EX.prog_cnt;
    Register_EX_MEM.opcode = Register_ID_EX.opcode;
    Register_EX_MEM.type = Register_ID_EX.type;

    if (Register_ID_EX.load_use_hazard){
        fout << ARM_OPC_NAME[BUBBLE] << endl;
        return;
    }

    //don't need to ex, branch and mov 
    if (Register_ID_EX.opcode == OPC_B || Register_ID_EX.opcode == OPC_BL || Register_ID_EX.opcode == OPC_EXIT){
        fout << ARM_OPC_NAME[Register_EX_MEM.opcode] << endl;
        return;
    }
    int r1; // first oprend
    bool oneIsPassed = false; //first oprend is from forwarding
    int r2; // second oprend
    bool twoIsPassed = false; // second oprend is from forwarding
    // preverse instruction
    if (EX_MEM_RegWrite && EX_MEM_Dest == Register_ID_EX.r1){
        r1 = EX_MEM_Result;
        oneIsPassed = true;
    }
    //preverse second instruction
    else if (MEM_WB_RegWrite && MEM_WB_Dest == Register_ID_EX.r1){
        r1 = MEM_WB_Result;
        oneIsPassed = true;
    }

    if (!oneIsPassed){
        r1 = Register_ID_EX.r1Data;
    }

    if (EX_MEM_RegWrite && EX_MEM_Dest == Register_ID_EX.r2){
        r2 = EX_MEM_Result;
        twoIsPassed = true;
    }else if (MEM_WB_RegWrite && MEM_WB_Dest == Register_ID_EX.r2){
        r2 = MEM_WB_Result;
        twoIsPassed = true;
    }
    
    if (!twoIsPassed){
        r2 = Register_ID_EX.r2Data;
    }

    int op1 = r1;
    int op2 = Register_ID_EX.type == INSTR_TYPE_IMM ? Register_ID_EX.immidiate : r2;

    //calculate
    int ans = 0;
    if (Register_ID_EX.opcode == OPC_ADD){
        ans = op1 + op2; 
        Register_EX_MEM.val_arith = ans;
    }else if (Register_ID_EX.opcode == OPC_SUB){
        ans = op1 - op2;
        Register_EX_MEM.val_arith = ans;
    }else if (Register_ID_EX.opcode == OPC_MUL){
        ans = op1 * op2;
        Register_EX_MEM.val_arith = ans;
    }else if (Register_ID_EX.opcode == OPC_MOV){
        ans = op2;
        Register_EX_MEM.val_arith = ans;
    }else if (Register_ID_EX.opcode == OPC_LDR || Register_ID_EX.opcode == OPC_STR){
        ans = op1 + op2;
        Register_EX_MEM.val_arith = ans;
    }else if (Register_ID_EX.opcode == OPC_CMPBGE || Register_ID_EX.opcode == OPC_CMPBNE){
        ans = op1 - op2;
        Register_EX_MEM.val_arith = ans;
        Register_EX_MEM.zero = ans == 0;
    }

    Register_EX_MEM.r1Data = r1;
    Register_EX_MEM.r2Data = r2;

    fout << ARM_OPC_NAME[Register_EX_MEM.opcode] << endl;
}

/**
 * Memory
*/
void MEM() {

    /*************************************/
    /* TODO: Fix and complete MEM stage. */
    /* Note: you are expected to detect  */
    /*  control hazards in this stage.   */
    /*************************************/
    if (Register_EX_MEM.load_use_hazard){
        fout << ARM_OPC_NAME[BUBBLE] << endl;
        return;
    }

    EX_MEM_RegWrite = false;
    EX_MEM_Dest = 0;
    EX_MEM_Result = 0;
    if (Register_ID_EX.opcode == OPC_ADD){
        EX_MEM_RegWrite = true;
        EX_MEM_Dest = Register_EX_MEM.dest;
        EX_MEM_Result = Register_EX_MEM.val_arith;
        Register_MEM_WB.val_data = Register_EX_MEM.val_arith;
    }else if (Register_ID_EX.opcode == OPC_SUB){
        EX_MEM_RegWrite = true;
        EX_MEM_Dest = Register_EX_MEM.dest;
        EX_MEM_Result = Register_EX_MEM.val_arith;
        Register_MEM_WB.val_data = Register_EX_MEM.val_arith;
    }else if (Register_ID_EX.opcode == OPC_MUL){
        EX_MEM_RegWrite = true;
        EX_MEM_Dest = Register_EX_MEM.dest;
        EX_MEM_Result = Register_EX_MEM.val_arith;
        Register_MEM_WB.val_data = Register_EX_MEM.val_arith;
    }else if (Register_ID_EX.opcode == OPC_MOV){
        EX_MEM_RegWrite = true;
        EX_MEM_Dest = Register_EX_MEM.dest;
        EX_MEM_Result = Register_EX_MEM.val_arith;
        Register_MEM_WB.val_data = Register_EX_MEM.val_arith;       
    }else if (Register_ID_EX.opcode == OPC_LDR){
        EX_MEM_RegWrite = true;
        EX_MEM_Dest = Register_EX_MEM.dest;
        EX_MEM_Result = Register_EX_MEM.val_arith;
        Register_MEM_WB.val_data = mem[Register_EX_MEM.val_arith];      
    }else if (Register_EX_MEM.opcode == OPC_STR){
        mem[Register_EX_MEM.val_arith] = Register_EX_MEM.r2Data;
    }

    Register_MEM_WB.dest = Register_EX_MEM.dest;
    Register_MEM_WB.prog_cnt = Register_EX_MEM.prog_cnt;
    Register_MEM_WB.opcode = Register_EX_MEM.opcode;
    Register_MEM_WB.type = Register_EX_MEM.type;
    fout << ARM_OPC_NAME[Register_EX_MEM.opcode] << endl;
}

/**
 * Write Back
*/
void WB() {

    /*************************************/
    /* TODO: Fix and complete WB stage.  */
    /* Hint: update the register state.  */
    /*************************************/
    if (load_use_hazard){
        fout << ARM_OPC_NAME[BUBBLE] << endl;
        return;
    }

    MEM_WB_RegWrite = false;
    MEM_WB_Dest = 0;
    MEM_WB_Result = 0;
    if (Register_MEM_WB.opcode == OPC_ADD){
        MEM_WB_RegWrite = true;
        MEM_WB_Dest = Register_MEM_WB.dest;
        MEM_WB_Result = Register_MEM_WB.val_data;
        reg[Register_MEM_WB.dest] = Register_MEM_WB.val_data; 
    }else if (Register_MEM_WB.opcode == OPC_SUB){
        MEM_WB_RegWrite = true;
        MEM_WB_Dest = Register_MEM_WB.dest;
        MEM_WB_Result = Register_MEM_WB.val_data;
        reg[Register_MEM_WB.dest] = Register_MEM_WB.val_data;
    }else if (Register_MEM_WB.opcode == OPC_MUL){
        MEM_WB_RegWrite = true;
        MEM_WB_Dest = Register_MEM_WB.dest;
        MEM_WB_Result = Register_MEM_WB.val_data;
        reg[Register_MEM_WB.dest] = Register_MEM_WB.val_data;
    }else if (Register_MEM_WB.opcode == OPC_MOV){
        MEM_WB_RegWrite = true;
        MEM_WB_Dest = Register_MEM_WB.dest;
        MEM_WB_Result = Register_MEM_WB.val_data;
        reg[Register_MEM_WB.dest] = Register_MEM_WB.val_data;       
    }else if (Register_MEM_WB.opcode == OPC_LDR){
        MEM_WB_RegWrite = true;
        MEM_WB_Dest = Register_MEM_WB.dest;
        MEM_WB_Result = Register_MEM_WB.val_data;
        reg[Register_MEM_WB.dest] = Register_MEM_WB.val_data;      
    }

    if (Register_MEM_WB.opcode == OPC_EXIT) {
        shut_down = true;
    } else if (Register_MEM_WB.opcode == END_OF_FILE) {
        file_end = true;
    }
    fout << ARM_OPC_NAME[Register_MEM_WB.opcode] << endl;
}

// ====================== PIPELINE STAGES ======================


void deal_with_hazards() {
    if (load_use_hazard) {

        /*******************************************************************/
        /* TODO: additional operations to deal with load-use data hazards. */
        /* Hint: inserting a bubble between load and use.                  */
        /*******************************************************************/
        reg[ARM_REG_PC] -= 4;

        load_use_hazard = false;
    }
    if (control_hazard) {

        /*******************************************************************/
        /* TODO: additional operations to deal with control hazards.       */
        /* Hint: roll back IF & ID & EX stages; reset PC to correct value. */
        /*******************************************************************/

        control_hazard = false;
    }
}

void print_register() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int num = 4 * i + j;
            char reg_str[10], eq_str[10];
            sprintf(reg_str, "reg[%d]", num);
            if (num < ARM_REG_LR) {
                sprintf(eq_str, "= %d", reg[num]);
            } else {
                sprintf(eq_str, "= 0x%x", reg[num]);
            }
            fout << left << setw(10) << reg_str << setw(12) << eq_str << "\t";
        }
        fout << endl;
    }
    fout << endl;
}

int compute_latency() {
    vector<int> latency_pipe(5, 0);
    // IF
    latency_pipe[0] = 1;
    // ID
    latency_pipe[1] = 1;
    // EX
    if (Register_ID_EX.opcode > OPC_INVALID && Register_ID_EX.opcode <= OPC_MOV) {
        latency_pipe[2] = latencies[Register_ID_EX.opcode - 1];
    } else if (Register_ID_EX.opcode == OPC_LDR && Register_ID_EX.type == INSTR_TYPE_EXTRA) {
        latency_pipe[2] = latencies[5];
    } else if (Register_ID_EX.opcode >= OPC_CMPBNE && Register_ID_EX.opcode <= OPC_CMPBGE) {
        latency_pipe[2] = latencies[7];
    } else {
        latency_pipe[2] = 1;
    }
    // MEM
    if (Register_EX_MEM.opcode == OPC_LDR) {
        latency_pipe[3] = latencies[4];
    } else if (Register_EX_MEM.opcode == OPC_STR) {
        latency_pipe[3] = latencies[6];
    } else if (Register_EX_MEM.opcode >= OPC_CMPBNE && Register_EX_MEM.opcode <= OPC_B) {
        latency_pipe[3] = latencies[Register_EX_MEM.opcode + 1];
    } else {
        latency_pipe[3] = 1;
    }
    // WB
    latency_pipe[4] = 1;
    return *max_element(latency_pipe.begin(), latency_pipe.end());
}

int main() {

    if (parse_file(latencies, instructions)) {
        return -1;
    }

    int instr_num = 0, latency = 0;

    fout.open("result.txt");
    print_register();

    while (!shut_down && !file_end) {
        if (Register_MEM_WB.opcode >= OPC_ADD && Register_MEM_WB.opcode <= OPC_B) {
            instr_num++;
        }

        fout << "======= WB =======" << endl;   WB();
        fout << "======= MEM ======" << endl;   MEM();
        fout << "======= EX =======" << endl;   EX();
        fout << "======= ID =======" << endl;   ID();
        fout << "======= IF =======" << endl;   IF();

        deal_with_hazards();

        fout << endl;
        print_register();

        latency += compute_latency();

        fout << "Instr N: " << instr_num << endl;
        fout << "Latency: " << latency << endl << endl << endl;
    }

    fout << "CPI: " << float(latency) / float(instr_num) << endl;
    fout.close();

}