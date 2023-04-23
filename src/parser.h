#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <bits/stdc++.h>
#include <string>
#include <vector>
#include <algorithm>

#include "arm_instr.h"

using namespace std;

static const string latency_path = "./latency.txt";
static const string instruction_path = "./instruction.txt";

int parse_file(vector<int> &latencies, vector<Instruction> &instructions);

#endif // PARSER_H
