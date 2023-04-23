#include "parser.h"

// ====================== LATENCY PARSING ======================

/**
 * Get the latency for each instruction from vector "lexeme",
 *   and put it to the right position of vector "latencies".
 * If there is an instruction that does not find the corresponding latency, 
 *   return an error.
*/
int parse_latency(vector<string> &lexeme, vector<int> &latencies) {
    int error = 0;

    const string latency_instr[12] = {
            "add", "sub", "mul", "mov",
            "ldr", "ldr_pseudo", "str",
            "cmp", "bne", "bge",
            "bl", "b"
    };

    for (int i = 0; i < 12; i++) {
        int instr_found = 0;
        for (int j = 0; j < lexeme.size(); j++) {
            if (latency_instr[i] == lexeme[j]) {
                latencies[i] = stoi(lexeme[j + 1]);
                instr_found = 1;
            }
        }
        if (!instr_found) {
            cout << "Error in " << latency_instr[i] << " latency" << endl;
            error++;
        }
    }

    return error;
}

/**
 * Read the latency file and get the latency of each instruction.
*/
int read_latency(vector<int> &latencies) {
    vector<string> lexeme;
    string s;
    int comment = 0;

    ifstream fin;
    fin.open(latency_path);
    while (!fin.eof()) {
        char c;
        fin.get(c);

        if (c == '\n') {
            comment = 0;
        } else if (c == '#') {
            comment = 1;
        }
        if (comment) {
            continue;
        }

        if (isalnum(c) || c == '_') {
            s.append(1, c);
        } else if (!s.empty()) {
            lexeme.push_back(s);
            s = "";
        }
    }
    fin.close();

    return parse_latency(lexeme, latencies);
}

// ====================== LATENCY PARSING ======================


// ==================== INSTRUCTION PARSING ====================

/**
 * Classify no-label instructions, including:
 * [add, sub, mul, mov, ldr, str]
 * Return the detail type of the instruction opcode.
*/
ARM_OPC_TYPE classify_no_label(vector<string> &lexeme, int curr_lex) {
    string lex_lower = lexeme[curr_lex];
    transform(lex_lower.begin(), lex_lower.end(), lex_lower.begin(), ::tolower);

    if (lex_lower == "add") {
        // add xx, xxx, ...
        if (lexeme[curr_lex + 2] == "," && lexeme[curr_lex + 4] == ",") {
            // add rd, reg1, ...
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3][0] == 'r' && stoi(lexeme[curr_lex + 3].substr(1)) < ARM_REG_SP) {
                // 1. add rd, reg1, reg2
                if (lexeme[curr_lex + 5][0] == 'r' && stoi(lexeme[curr_lex + 5].substr(1)) < ARM_REG_SP) {
                    return OPC_TYPE_ADD_REG;
                }
                // 2. add rd, reg1, #imm
                if (lexeme[curr_lex + 5][0] == '#') {
                    return OPC_TYPE_ADD_IMM;
                }
            }
            // 3. add sp, sp, #imm
            if (lexeme[curr_lex + 1] == "sp" && lexeme[curr_lex + 3] == "sp"
                && lexeme[curr_lex + 5][0] == '#') {
                return OPC_TYPE_ADD_SP_IMM;
            }
        }
    }

    if (lex_lower == "sub") {
        // sub xx, xxx, ...
        if (lexeme[curr_lex + 2] == "," && lexeme[curr_lex + 4] == ",") {
            // sub rd, reg1, ...
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3][0] == 'r' && stoi(lexeme[curr_lex + 3].substr(1)) < ARM_REG_SP) {
                // 4. sub rd, reg1, reg2
                if (lexeme[curr_lex + 5][0] == 'r' && stoi(lexeme[curr_lex + 5].substr(1)) < ARM_REG_SP) {
                    return OPC_TYPE_SUB_REG;
                }
                // 5. sub rd, reg1, #imm
                if (lexeme[curr_lex + 5][0] == '#') {
                    return OPC_TYPE_SUB_IMM;
                }
            }
            // 6. sub sp, sp, #imm
            if (lexeme[curr_lex + 1] == "sp" && lexeme[curr_lex + 3] == "sp"
                && lexeme[curr_lex + 5][0] == '#') {
                return OPC_TYPE_SUB_SP_IMM;
            }
        }
    }

    if (lex_lower == "mul") {
        // mul xx, xxx, ...
        if (lexeme[curr_lex + 2] == "," && lexeme[curr_lex + 4] == ",") {
            // mul rd, reg1, ...
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3][0] == 'r' && stoi(lexeme[curr_lex + 3].substr(1)) < ARM_REG_SP) {
                // 7. mul rd, reg1, reg2
                if (lexeme[curr_lex + 5][0] == 'r' && stoi(lexeme[curr_lex + 5].substr(1)) < ARM_REG_SP) {
                    return OPC_TYPE_MUL_REG;
                }
                // 8. mul rd, reg1, #imm
                if (lexeme[curr_lex + 5][0] == '#') {
                    return OPC_TYPE_MUL_IMM;
                }
            }
        }
    }

    if (lex_lower == "mov") {
        // mov xx, ...
        if (lexeme[curr_lex + 2] == ",") {
            // 9. mov rd, reg1
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3][0] == 'r' && stoi(lexeme[curr_lex + 3].substr(1)) < ARM_REG_SP) {
                return OPC_TYPE_MOV_REG;
            }
            // 10. mov pc, lr
            if (lexeme[curr_lex + 1] == "pc" && lexeme[curr_lex + 3] == "lr") {
                return OPC_TYPE_MOV_PC_LR;
            }
            // 11. mov rd, #imm
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3][0] == '#') {
                return OPC_TYPE_MOV_IMM;
            }
        }
    }

    if (lex_lower == "ldr") {
        // ldr xx, ...
        if (lexeme[curr_lex + 2] == ",") {
            // 12. ldr rd, =label
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3][0] == '=') {
                return OPC_TYPE_LDR_LABEL;
            }
            // 13. ldr rd, [reg1]
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3] == "[" && lexeme[curr_lex + 5] == "]"
                && lexeme[curr_lex + 4][0] == 'r' && stoi(lexeme[curr_lex + 4].substr(1)) < ARM_REG_SP) {
                return OPC_TYPE_LDR_REG;
            }
            // ldr xx, [xxx, xxx] ...
            if (lexeme[curr_lex + 3] == "[" && lexeme[curr_lex + 5] == "," && lexeme[curr_lex + 7] == "]") {
                // 14. ldr rd, [reg1, #imm]
                if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                    && lexeme[curr_lex + 4][0] == 'r' && stoi(lexeme[curr_lex + 4].substr(1)) < ARM_REG_SP
                    && lexeme[curr_lex + 6][0] == '#') {
                    return OPC_TYPE_LDR_REG_OFFSET;
                }
                // 15. ldr rd, [sp, #imm]
                if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                    && lexeme[curr_lex + 4] == "sp" && lexeme[curr_lex + 6][0] == '#') {
                    return OPC_TYPE_LDR_SP_OFFSET;
                }
                // 16. ldr lr, [sp, #imm]
                if (lexeme[curr_lex + 1] == "lr" && lexeme[curr_lex + 4] == "sp"
                    && lexeme[curr_lex + 6][0] == '#') {
                    return OPC_TYPE_LDR_LR_SP_OFFSET;
                }
            }
        }
    }

    if (lex_lower == "str") {
        // str xx, ...
        if (lexeme[curr_lex + 2] == ",") {
            // 17. str rd, [reg1]
            if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                && lexeme[curr_lex + 3] == "[" && lexeme[curr_lex + 5] == "]"
                && lexeme[curr_lex + 4][0] == 'r' && stoi(lexeme[curr_lex + 4].substr(1)) < ARM_REG_SP) {
                return OPC_TYPE_STR_REG;
            }
            // str xx, [xxx, xxx] ...
            if (lexeme[curr_lex + 3] == "[" && lexeme[curr_lex + 5] == "," && lexeme[curr_lex + 7] == "]") {
                // 18. str rd, [reg1, #imm]
                if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                    && lexeme[curr_lex + 4][0] == 'r' && stoi(lexeme[curr_lex + 4].substr(1)) < ARM_REG_SP
                    && lexeme[curr_lex + 6][0] == '#') {
                    return OPC_TYPE_STR_REG_OFFSET;
                }
                // 19. str rd, [sp, #imm]
                if (lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP
                    && lexeme[curr_lex + 4] == "sp" && lexeme[curr_lex + 6][0] == '#') {
                    return OPC_TYPE_STR_SP_OFFSET;
                }
                // 20. str lr, [sp, #imm]
                if (lexeme[curr_lex + 1] == "lr" && lexeme[curr_lex + 4] == "sp"
                    && lexeme[curr_lex + 6][0] == '#') {
                    return OPC_TYPE_STR_LR_SP_OFFSET;
                }
            }
        }
    }

    return OPC_TYPE_INVALID;
}

/**
 * Classify compare instructions, including:
 * [cmp & bne, cmp & bge]
 * Return the detail type of the instruction opcode.
*/
ARM_OPC_TYPE classify_cmp(vector<string> &lexeme, int curr_lex) {
    string lex_lower = lexeme[curr_lex + 4];
    transform(lex_lower.begin(), lex_lower.end(), lex_lower.begin(), ::tolower);

    // cmp reg1, xxx bne/bge ...
    if (lex_lower == "bne" || lex_lower == "bge" && lexeme[curr_lex + 2] == ","
                              && lexeme[curr_lex + 1][0] == 'r' && stoi(lexeme[curr_lex + 1].substr(1)) < ARM_REG_SP) {
        // cmp reg1, reg2 bne/bge ...
        if (lexeme[curr_lex + 3][0] == 'r' && stoi(lexeme[curr_lex + 3].substr(1)) < ARM_REG_SP) {
            // 21. cmp reg1, reg2 bne label
            if (lex_lower == "bne") {
                return OPC_TYPE_CMP_BNE_REG;
            }
            // 23. cmp reg1, reg2 bge label
            if (lex_lower == "bge") {
                return OPC_TYPE_CMP_BGE_REG;
            }
        }
        // cmp reg1, #imm bne/bge ...
        if (lexeme[curr_lex + 3][0] == '#') {
            // 22. cmp reg1, #imm bne label
            if (lex_lower == "bne") {
                return OPC_TYPE_CMP_BNE_IMM;
            }
            // 24. cmp reg1, #imm bge label
            if (lex_lower == "bge") {
                return OPC_TYPE_CMP_BGE_IMM;
            }
        }
    }

    return OPC_TYPE_INVALID;
}

/**
 * Get the instructions from vector "lexeme",
 *   and put them to the vector "instructions".
 * If there is an instruction that does not match the correct format, 
 *   return an error.
*/
int parse_instruction(vector<string> lexeme, vector<Instruction> &instructions) {
    int error = 0;
    int lex_idx = 0;

    int mem_alloc = 0;

    vector<symbol> symbol_table;
    vector<symbol> symbol_temps;

    while (lex_idx < lexeme.size()) {
        string lex_lower = lexeme[lex_idx];
        transform(lex_lower.begin(), lex_lower.end(), lex_lower.begin(), ::tolower);

        if (lex_lower == "add" || lex_lower == "sub" || lex_lower == "mul"
            || lex_lower == "mov" || lex_lower == "ldr" || lex_lower == "str") {
            ARM_OPC_TYPE opc_type = classify_no_label(lexeme, lex_idx);

            if (opc_type == OPC_TYPE_INVALID) {
                cout << "Error in " << lexeme[lex_idx] << " instruction" << endl;
                lex_idx++;
                error++;
            } else {
                Instruction I;
                symbol sym;

                switch (opc_type) {
                    case OPC_TYPE_ADD_REG:
                        I.opcode = OPC_ADD;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_REG;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_ADD_IMM:
                        I.opcode = OPC_ADD;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_ADD_SP_IMM:
                        I.opcode = OPC_ADD;
                        I.dest = ARM_REG_SP;
                        I.operand1 = ARM_REG_SP;
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_SUB_REG:
                        I.opcode = OPC_SUB;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_REG;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_SUB_IMM:
                        I.opcode = OPC_SUB;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_SUB_SP_IMM:
                        I.opcode = OPC_SUB;
                        I.dest = ARM_REG_SP;
                        I.operand1 = ARM_REG_SP;
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_MUL_REG:
                        I.opcode = OPC_MUL;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_REG;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_MUL_IMM:
                        I.opcode = OPC_MUL;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 5].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_MOV_REG:
                        I.opcode = OPC_MOV;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.type = INSTR_TYPE_REG;
                        lex_idx += 4;
                        break;
                    case OPC_TYPE_MOV_IMM:
                        I.opcode = OPC_MOV;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 4;
                        break;
                    case OPC_TYPE_MOV_PC_LR:
                        I.opcode = OPC_MOV;
                        I.type = INSTR_TYPE_EXTRA;
                        lex_idx += 4;
                        break;
                    case OPC_TYPE_LDR_LABEL:
                        I.opcode = OPC_LDR;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.type = INSTR_TYPE_EXTRA;
                        sym.type = SYMBOL_LDR_LABEL;
                        sym.name = lexeme[lex_idx + 3].substr(1);
                        sym.pos = instructions.size();
                        symbol_temps.push_back(sym);
                        lex_idx += 4;
                        break;
                    case OPC_TYPE_LDR_REG:
                        I.opcode = OPC_LDR;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 4].substr(1));
                        I.type = INSTR_TYPE_REG;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_LDR_REG_OFFSET:
                        I.opcode = OPC_LDR;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 4].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 6].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 8;
                        break;
                    case OPC_TYPE_LDR_SP_OFFSET:
                        I.opcode = OPC_LDR;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = ARM_REG_SP;
                        I.operand2 = stoi(lexeme[lex_idx + 6].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 8;
                        break;
                    case OPC_TYPE_LDR_LR_SP_OFFSET:
                        I.opcode = OPC_LDR;
                        I.dest = ARM_REG_LR;
                        I.operand1 = ARM_REG_SP;
                        I.operand2 = stoi(lexeme[lex_idx + 6].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 8;
                        break;
                    case OPC_TYPE_STR_REG:
                        I.opcode = OPC_STR;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 4].substr(1));
                        I.type = INSTR_TYPE_REG;
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_STR_REG_OFFSET:
                        I.opcode = OPC_STR;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = stoi(lexeme[lex_idx + 4].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 6].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 8;
                        break;
                    case OPC_TYPE_STR_SP_OFFSET:
                        I.opcode = OPC_STR;
                        I.dest = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand1 = ARM_REG_SP;
                        I.operand2 = stoi(lexeme[lex_idx + 6].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 8;
                        break;
                    case OPC_TYPE_STR_LR_SP_OFFSET:
                        I.opcode = OPC_STR;
                        I.dest = ARM_REG_LR;
                        I.operand1 = ARM_REG_SP;
                        I.operand2 = stoi(lexeme[lex_idx + 6].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        lex_idx += 8;
                        break;
                    default:
                        cout << "Error in " << lexeme[lex_idx] << " instruction" << endl;
                        break;
                }

                if (I.opcode != OPC_INVALID) {
                    instructions.push_back(I);
                }
            }

            continue;
        }

        if (lex_lower == "cmp") {
            ARM_OPC_TYPE opc_type = classify_cmp(lexeme, lex_idx);

            if (opc_type == OPC_TYPE_INVALID) {
                cout << "Error in " << lexeme[lex_idx] << " instruction" << endl;
                lex_idx++;
                error++;
            } else {
                Instruction I;
                symbol sym;

                switch (opc_type) {
                    case OPC_TYPE_CMP_BNE_REG:
                        I.opcode = OPC_CMPBNE;
                        I.operand1 = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.type = INSTR_TYPE_REG;
                        sym.type = SYMBOL_JMP_LABEL;
                        sym.name = lexeme[lex_idx + 5];
                        sym.pos = instructions.size();
                        symbol_temps.push_back(sym);
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_CMP_BNE_IMM:
                        I.opcode = OPC_CMPBNE;
                        I.operand1 = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        sym.type = SYMBOL_JMP_LABEL;
                        sym.name = lexeme[lex_idx + 5];
                        sym.pos = instructions.size();
                        symbol_temps.push_back(sym);
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_CMP_BGE_REG:
                        I.opcode = OPC_CMPBGE;
                        I.operand1 = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.type = INSTR_TYPE_REG;
                        sym.type = SYMBOL_JMP_LABEL;
                        sym.name = lexeme[lex_idx + 5];
                        sym.pos = instructions.size();
                        symbol_temps.push_back(sym);
                        lex_idx += 6;
                        break;
                    case OPC_TYPE_CMP_BGE_IMM:
                        I.opcode = OPC_CMPBGE;
                        I.operand1 = stoi(lexeme[lex_idx + 1].substr(1));
                        I.operand2 = stoi(lexeme[lex_idx + 3].substr(1));
                        I.type = INSTR_TYPE_IMM;
                        sym.type = SYMBOL_JMP_LABEL;
                        sym.name = lexeme[lex_idx + 5];
                        sym.pos = instructions.size();
                        symbol_temps.push_back(sym);
                        lex_idx += 6;
                        break;
                    default:
                        cout << "Error in " << lexeme[lex_idx] << " instruction" << endl;
                        break;
                }

                if (I.opcode != OPC_INVALID) {
                    instructions.push_back(I);
                }
            }

            continue;
        }

        if (lex_lower == "bl" || lex_lower == "b") {
            Instruction I;
            symbol sym;

            if (lex_lower == "bl") {
                I.opcode = OPC_BL;
            }
            if (lex_lower == "b") {
                I.opcode = OPC_B;
            }

            sym.type = SYMBOL_JMP_LABEL;
            sym.name = lexeme[lex_idx + 1];
            sym.pos = instructions.size();
            symbol_temps.push_back(sym);

            lex_idx += 2;
            instructions.push_back(I);
            continue;
        }

        if (lex_lower == ":") {
            symbol sym;

            sym.name = lexeme[lex_idx - 1];
            if (lexeme[lex_idx + 1] == ".space") {
                sym.type = SYMBOL_DATA_LABEL;
                sym.pos = mem_alloc;
                mem_alloc = mem_alloc + stoi(lexeme[lex_idx + 2]);
            } else {
                sym.type = SYMBOL_JMP_LABEL;
                sym.pos = instructions.size();
            }
            symbol_table.push_back(sym);

            lex_idx++;
            continue;
        }

        if (lex_lower == "exit") {
            Instruction I;
            I.opcode = OPC_EXIT;
            instructions.push_back(I);
            lex_idx++;
            continue;
        }

        lex_idx++;
    }

    // Parse symbols
    for (auto &tmp: symbol_temps) {
        for (auto &sym: symbol_table) {
            if (tmp.name == sym.name) {
                if (tmp.type == SYMBOL_JMP_LABEL) {
                    instructions[tmp.pos].dest = sym.pos;
                } else {
                    instructions[tmp.pos].operand1 = sym.pos;
                }
            }
        }
    }

    return error;
}

/**
 * Read the instruction file and get the instruction flow.
*/
int read_instruction(vector<Instruction> &instructions) {
    vector<string> lexeme;
    string s;
    int comment = 0;

    ifstream fin;
    fin.open(instruction_path);
    while (!fin.eof()) {
        char c;
        fin.get(c);

        if (c == '\n') {
            comment = 0;
        } else if (c == '@') {
            comment = 1;
        }
        if (comment) {
            continue;
        }

        if (isalnum(c) || c == '_' || c == '.' || c == '#' || c == '=') {
            s.append(1, c);
        } else {
            if (!s.empty()) {
                lexeme.push_back(s);
                s = "";
            }
            if (c == ':' || c == ',' || c == '[' || c == ']') {
                lexeme.emplace_back(1, c);
            }
        }
    }
    fin.close();

    return parse_instruction(lexeme, instructions);
}

// ==================== INSTRUCTION PARSING ====================

/**
 * Read and parse latency file & instruction file.
*/
int parse_file(vector<int> &latencies, vector<Instruction> &instructions) {
    int err = 0;
    err += read_latency(latencies);
    err += read_instruction(instructions);
    return err;
}
