#pragma once
#include "mpc/mpc.h"

void parser_initialize();
void parser_cleanup();
int parse(const char* filename, const char* string, mpc_result_t* result);
int parse_contents(const char* filename, mpc_result_t* result);
