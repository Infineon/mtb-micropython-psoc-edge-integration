/****************************************************************************
* File Name        : Chess_config.c
*
* Description      : This source file contains the configuration object for WWD and NLU
*
* Related Document : See README.md
*
*****************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company)
* All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*****************************************************************************/

#include "Chess_config.h"

#include "mtb_ml.h"
#include "mtb_ml_model_16x8.h"
#include "AM_LSTM_tflm_model_int16x8.h"

#include "ifx_va_prms.h"
#include "ifx_sp_common_priv.h"

#include "Chess_U55_WWmodel.h"
#include "Chess_U55_CMDmodel.h"
#include "U55_NMBmodel.h"

#include "Chess.h"
#include "Chess_ifx_va_config_prms.h"

/* Following am_tensor_arena has been counted as part of persistent memory total size */
/* Tensor_arena buffer must be in SOCMEM and aligned by 16 which are required by U55 */
static uint8_t am_tensor_arena[AM_LSTM_ARENA_SIZE] __attribute__((aligned(16)))
                                          __attribute__((section(".cy_socmem_data")));


static int16_t data_feed_int[N_SEQ * FEATURE_BUF_SZ] __attribute__((aligned(16)));
static float mtb_ml_input_buffer[N_SEQ * FEATURE_BUF_SZ];

static float xIn[FRAME_SIZE_16K] __attribute__((section(".wwd_nlu_data3")));
static float features[FEATURE_BUF_SZ] __attribute__((section(".wwd_nlu_data4")));
static float output_scores[(N_PHONEMES + 1) * (1 + AM_LOOKBACK)] __attribute__((section(".wwd_nlu_data5")));

//common buffers
static mtb_wwd_nlu_buff_t wwd_nlu_buff =
{
    .am_model_bin = { MTB_ML_MODEL_BIN_DATA(AM_LSTM) },
    .am_model_buffer = {
        .tensor_arena = am_tensor_arena,
        .tensor_arena_size = AM_LSTM_ARENA_SIZE
    },
    .data_feed_int = data_feed_int,
    .mtb_ml_input_buffer = mtb_ml_input_buffer,
    .output_scores = output_scores,
    .xIn = xIn,
    .features = features
};

// NLU setup array
static mtb_nlu_setup_array_t nlu_setup_array =
{
    .intent_name_list = Chess_intent_name_list,
    .variable_name_list = Chess_variable_name_list,
    .variable_phrase_list = Chess_variable_phrase_list,
    .unit_phrase_list = Chess_unit_phrase_list,
    .intent_map_array = Chess_intent_map_array,
    .intent_map_array_sizes = Chess_intent_map_array_sizes,
    .variable_phrase_sizes = Chess_variable_phrase_sizes,
    .unit_phrase_map_array = Chess_unit_phrase_map_array,
    .unit_phrase_map_array_sizes = Chess_unit_phrase_map_array_sizes,
    .NUM_UNIT_PHRASES = sizeof(Chess_unit_phrase_list),
};

// WW config
static mtb_wwd_conf_t ww_conf = {
    .ww_params = Chess_dfww_prms,
    .callback.cb_for_event = CY_EVENT_SOD,
    .callback.cb_function = Chess_wake_word_callback
};

// NLU config
static mtb_nlu_config_t nlu_conf = {
    .nlu_params = Chess_dfcmd_prms,
    .nlu_command_timeout = 5000,
};

static mtb_wwd_nlu_config_t ww_1_conf = {
    .ww_model_ptr = Chess_WWmodeldata,
    .cmd_model_ptr = Chess_CMDmodeldata,
    .nmb_model_ptr = NMBmodeldata,
    .wwd_nlu_buff_data = &wwd_nlu_buff,
    .sod_params = Chess_sod_prms,
    .hpf_params = Chess_pre_proc_hpf_prms,
    .denoise_params = Chess_denoise_prms,
    .ww_conf = &ww_conf,
    .nlu_conf.nlu_config = &nlu_conf,
    .nlu_conf.nlu_variable_data = &nlu_setup_array,
};

mtb_wwd_nlu_config_t *Chess_ww_nlu_configs[CHESS_NO_OF_WAKE_WORD] = {&ww_1_conf
};

char *Chess_ww_str[CHESS_NO_OF_WAKE_WORD] = {"OK chess"};

const char* Chess_intent_name_list[CHESS_NUM_INTENTS] = {
    "moveByFigure",
};

const char* Chess_variable_name_list[CHESS_NUM_VARIABLES] = {
    "Figure",
    "Letter",
    "Number",
};

const char* Chess_variable_phrase_list[CHESS_NUM_VARIABLE_PHRASES] = {
    "King", "Pawn", "Bishop", "Knight", "Rook", "Queen", // Figure
    "A", "B", "C", "D", "E", "F", "G", // Letter
    "", // Number
};

const char* Chess_unit_phrase_list[CHESS_NUM_UNIT_PHRASES] = {
    "degree", "degrees", 
    "percent", 
    "level", "levels", 
    "hour", "hours", 
    "minute", "minutes", 
    "second", "seconds", 
    "day", "days", 
    "", 
    "AM", 
    "PM", 
};

const int Chess_intent_map_array[CHESS_INTENT_MAP_ARRAY_TOTAL_SIZE] = {
    0, 3, 0, 0, 1, 0, 2, -1, // (King) to (A) <numbers9> {}
    0, 3, 0, 0, 1, 1, 2, -1, // (King) to (B) <numbers9> {}
    0, 3, 0, 0, 1, 2, 2, -1, // (King) to (C) <numbers9> {}
    0, 3, 0, 0, 1, 3, 2, -1, // (King) to (D) <numbers9> {}
    0, 3, 0, 0, 1, 4, 2, -1, // (King) to (E) <numbers9> {}
    0, 3, 0, 0, 1, 5, 2, -1, // (King) to (F) <numbers9> {}
    0, 3, 0, 0, 1, 6, 2, -1, // (King) to (G) <numbers9> {}
    0, 3, 0, 1, 1, 0, 2, -1, // (Pawn) to (A) <numbers9> {}
    0, 3, 0, 1, 1, 1, 2, -1, // (Pawn) to (B) <numbers9> {}
    0, 3, 0, 1, 1, 2, 2, -1, // (Pawn) to (C) <numbers9> {}
    0, 3, 0, 1, 1, 3, 2, -1, // (Pawn) to (D) <numbers9> {}
    0, 3, 0, 1, 1, 4, 2, -1, // (Pawn) to (E) <numbers9> {}
    0, 3, 0, 1, 1, 5, 2, -1, // (Pawn) to (F) <numbers9> {}
    0, 3, 0, 1, 1, 6, 2, -1, // (Pawn) to (G) <numbers9> {}
    0, 3, 0, 2, 1, 0, 2, -1, // (Bishop) to (A) <numbers9> {}
    0, 3, 0, 2, 1, 1, 2, -1, // (Bishop) to (B) <numbers9> {}
    0, 3, 0, 2, 1, 2, 2, -1, // (Bishop) to (C) <numbers9> {}
    0, 3, 0, 2, 1, 3, 2, -1, // (Bishop) to (D) <numbers9> {}
    0, 3, 0, 2, 1, 4, 2, -1, // (Bishop) to (E) <numbers9> {}
    0, 3, 0, 2, 1, 5, 2, -1, // (Bishop) to (F) <numbers9> {}
    0, 3, 0, 2, 1, 6, 2, -1, // (Bishop) to (G) <numbers9> {}
    0, 3, 0, 3, 1, 0, 2, -1, // (Knight) to (A) <numbers9> {}
    0, 3, 0, 3, 1, 1, 2, -1, // (Knight) to (B) <numbers9> {}
    0, 3, 0, 3, 1, 2, 2, -1, // (Knight) to (C) <numbers9> {}
    0, 3, 0, 3, 1, 3, 2, -1, // (Knight) to (D) <numbers9> {}
    0, 3, 0, 3, 1, 4, 2, -1, // (Knight) to (E) <numbers9> {}
    0, 3, 0, 3, 1, 5, 2, -1, // (Knight) to (F) <numbers9> {}
    0, 3, 0, 3, 1, 6, 2, -1, // (Knight) to (G) <numbers9> {}
    0, 3, 0, 4, 1, 0, 2, -1, // (Rook) to (A) <numbers9> {}
    0, 3, 0, 4, 1, 1, 2, -1, // (Rook) to (B) <numbers9> {}
    0, 3, 0, 4, 1, 2, 2, -1, // (Rook) to (C) <numbers9> {}
    0, 3, 0, 4, 1, 3, 2, -1, // (Rook) to (D) <numbers9> {}
    0, 3, 0, 4, 1, 4, 2, -1, // (Rook) to (E) <numbers9> {}
    0, 3, 0, 4, 1, 5, 2, -1, // (Rook) to (F) <numbers9> {}
    0, 3, 0, 4, 1, 6, 2, -1, // (Rook) to (G) <numbers9> {}
    0, 3, 0, 5, 1, 0, 2, -1, // (Queen) to (A) <numbers9> {}
    0, 3, 0, 5, 1, 1, 2, -1, // (Queen) to (B) <numbers9> {}
    0, 3, 0, 5, 1, 2, 2, -1, // (Queen) to (C) <numbers9> {}
    0, 3, 0, 5, 1, 3, 2, -1, // (Queen) to (D) <numbers9> {}
    0, 3, 0, 5, 1, 4, 2, -1, // (Queen) to (E) <numbers9> {}
    0, 3, 0, 5, 1, 5, 2, -1, // (Queen) to (F) <numbers9> {}
    0, 3, 0, 5, 1, 6, 2, -1, // (Queen) to (G) <numbers9> {}
    0, 3, 0, 0, 1, 0, 2, -1, // move the (King) to (A) <numbers9> {}
    0, 3, 0, 0, 1, 1, 2, -1, // move the (King) to (B) <numbers9> {}
    0, 3, 0, 0, 1, 2, 2, -1, // move the (King) to (C) <numbers9> {}
    0, 3, 0, 0, 1, 3, 2, -1, // move the (King) to (D) <numbers9> {}
    0, 3, 0, 0, 1, 4, 2, -1, // move the (King) to (E) <numbers9> {}
    0, 3, 0, 0, 1, 5, 2, -1, // move the (King) to (F) <numbers9> {}
    0, 3, 0, 0, 1, 6, 2, -1, // move the (King) to (G) <numbers9> {}
    0, 3, 0, 1, 1, 0, 2, -1, // move the (Pawn) to (A) <numbers9> {}
    0, 3, 0, 1, 1, 1, 2, -1, // move the (Pawn) to (B) <numbers9> {}
    0, 3, 0, 1, 1, 2, 2, -1, // move the (Pawn) to (C) <numbers9> {}
    0, 3, 0, 1, 1, 3, 2, -1, // move the (Pawn) to (D) <numbers9> {}
    0, 3, 0, 1, 1, 4, 2, -1, // move the (Pawn) to (E) <numbers9> {}
    0, 3, 0, 1, 1, 5, 2, -1, // move the (Pawn) to (F) <numbers9> {}
    0, 3, 0, 1, 1, 6, 2, -1, // move the (Pawn) to (G) <numbers9> {}
    0, 3, 0, 2, 1, 0, 2, -1, // move the (Bishop) to (A) <numbers9> {}
    0, 3, 0, 2, 1, 1, 2, -1, // move the (Bishop) to (B) <numbers9> {}
    0, 3, 0, 2, 1, 2, 2, -1, // move the (Bishop) to (C) <numbers9> {}
    0, 3, 0, 2, 1, 3, 2, -1, // move the (Bishop) to (D) <numbers9> {}
    0, 3, 0, 2, 1, 4, 2, -1, // move the (Bishop) to (E) <numbers9> {}
    0, 3, 0, 2, 1, 5, 2, -1, // move the (Bishop) to (F) <numbers9> {}
    0, 3, 0, 2, 1, 6, 2, -1, // move the (Bishop) to (G) <numbers9> {}
    0, 3, 0, 3, 1, 0, 2, -1, // move the (Knight) to (A) <numbers9> {}
    0, 3, 0, 3, 1, 1, 2, -1, // move the (Knight) to (B) <numbers9> {}
    0, 3, 0, 3, 1, 2, 2, -1, // move the (Knight) to (C) <numbers9> {}
    0, 3, 0, 3, 1, 3, 2, -1, // move the (Knight) to (D) <numbers9> {}
    0, 3, 0, 3, 1, 4, 2, -1, // move the (Knight) to (E) <numbers9> {}
    0, 3, 0, 3, 1, 5, 2, -1, // move the (Knight) to (F) <numbers9> {}
    0, 3, 0, 3, 1, 6, 2, -1, // move the (Knight) to (G) <numbers9> {}
    0, 3, 0, 4, 1, 0, 2, -1, // move the (Rook) to (A) <numbers9> {}
    0, 3, 0, 4, 1, 1, 2, -1, // move the (Rook) to (B) <numbers9> {}
    0, 3, 0, 4, 1, 2, 2, -1, // move the (Rook) to (C) <numbers9> {}
    0, 3, 0, 4, 1, 3, 2, -1, // move the (Rook) to (D) <numbers9> {}
    0, 3, 0, 4, 1, 4, 2, -1, // move the (Rook) to (E) <numbers9> {}
    0, 3, 0, 4, 1, 5, 2, -1, // move the (Rook) to (F) <numbers9> {}
    0, 3, 0, 4, 1, 6, 2, -1, // move the (Rook) to (G) <numbers9> {}
    0, 3, 0, 5, 1, 0, 2, -1, // move the (Queen) to (A) <numbers9> {}
    0, 3, 0, 5, 1, 1, 2, -1, // move the (Queen) to (B) <numbers9> {}
    0, 3, 0, 5, 1, 2, 2, -1, // move the (Queen) to (C) <numbers9> {}
    0, 3, 0, 5, 1, 3, 2, -1, // move the (Queen) to (D) <numbers9> {}
    0, 3, 0, 5, 1, 4, 2, -1, // move the (Queen) to (E) <numbers9> {}
    0, 3, 0, 5, 1, 5, 2, -1, // move the (Queen) to (F) <numbers9> {}
    0, 3, 0, 5, 1, 6, 2, -1, // move the (Queen) to (G) <numbers9> {}
};

const int Chess_intent_map_array_sizes[CHESS_NUM_COMMANDS] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
    8, 8, 8, 8, 8, 8, 8, 8, 8, 
};

const int Chess_variable_phrase_sizes[CHESS_NUM_VARIABLES] = {
    6, // Figure: (King,Pawn,Bishop,Knight,Rook,Queen)
    7, // Letter: (A,B,C,D,E,F,G)
    0, // Number: None
};

const int Chess_unit_phrase_map_array[CHESS_UNIT_PHRASE_MAP_ARRAY_TOTAL_SIZE] = {
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
    1, 1, 13, // {}
};

const int Chess_unit_phrase_map_array_sizes[CHESS_NUM_COMMANDS] = {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 3, 
};



__attribute__((weak)) void Chess_wake_word_callback(mtb_wwd_nlu_events_t event)
{

}
