#pragma once
//------------------------------------------------------------------------------
//  LLM maintained.
//
//  capture_log.h
//
//  A silent logger for the functional tests. It stores the log item ids
//  and does not print them. Like slog_func, a panic (log level 0)
//  prints the message and aborts.
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPTURE_LOG_MAX_ITEMS (64)
static uint32_t capture_log_items[CAPTURE_LOG_MAX_ITEMS];
static int capture_log_num_items = 0;

static inline void capture_log_reset(void) {
    capture_log_num_items = 0;
    memset(capture_log_items, 0, sizeof(capture_log_items));
}

static void capture_log(const char* tag, uint32_t log_level, uint32_t log_item_id, const char* msg, uint32_t line_nr, const char* file, void* user_data) {
    (void)user_data;
    if (capture_log_num_items < CAPTURE_LOG_MAX_ITEMS) {
        capture_log_items[capture_log_num_items++] = log_item_id;
    }
    if (0 == log_level) {
        printf("[%s][panic][id:%u] %s:%u: %s\n", tag, log_item_id, file ? file : "", line_nr, msg ? msg : "");
        abort();
    }
}
