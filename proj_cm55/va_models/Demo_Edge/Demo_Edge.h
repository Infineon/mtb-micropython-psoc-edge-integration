/*****************************************************************************
* \file Demo_Edge.h
*****************************************************************************
* \copyright
* Copyright 2025, Infineon Technologies.
* All rights reserved.
*****************************************************************************/

#ifndef DEMO_EDGE_H
#define DEMO_EDGE_H

#include <stdint.h>

#define DEMO_EDGE_NUM_INTENTS 5
#define DEMO_EDGE_NUM_COMMANDS 5
#define DEMO_EDGE_NUM_VARIABLES 5
#define DEMO_EDGE_NUM_VARIABLE_PHRASES 5
#define DEMO_EDGE_NUM_UNIT_PHRASES 16
#define DEMO_EDGE_INTENT_MAP_ARRAY_TOTAL_SIZE 20
#define DEMO_EDGE_UNIT_PHRASE_MAP_ARRAY_TOTAL_SIZE 5

extern const char* Demo_Edge_intent_name_list[DEMO_EDGE_NUM_INTENTS];

extern const char* Demo_Edge_variable_name_list[DEMO_EDGE_NUM_VARIABLES];

extern const char* Demo_Edge_variable_phrase_list[DEMO_EDGE_NUM_VARIABLE_PHRASES];

extern const char* Demo_Edge_unit_phrase_list[DEMO_EDGE_NUM_UNIT_PHRASES];

extern const int Demo_Edge_intent_map_array[DEMO_EDGE_INTENT_MAP_ARRAY_TOTAL_SIZE];

extern const int Demo_Edge_intent_map_array_sizes[DEMO_EDGE_NUM_COMMANDS];

extern const int Demo_Edge_variable_phrase_sizes[DEMO_EDGE_NUM_VARIABLES];

extern const int Demo_Edge_unit_phrase_map_array[DEMO_EDGE_UNIT_PHRASE_MAP_ARRAY_TOTAL_SIZE];

extern const int Demo_Edge_unit_phrase_map_array_sizes[DEMO_EDGE_NUM_COMMANDS];

#endif // DEMO_EDGE_H
