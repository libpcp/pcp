/*
 *------------------------------------------------------------------
 * test_pcp_logger.c
 *
 * May, 2013, ptatrai
 *
 * Copyright (c) 2013-2013 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#include "default_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcp.h"
#include "pcp_logger.h"
#include "test_macro.h"

static int test_mode;
static char test_msg[10240];

static void test_logfn(pcp_debug_mode_t mode, const char* msg)
{
    test_mode = mode;
    strncpy(test_msg, msg, sizeof(test_msg)-1);
    test_msg[sizeof(test_msg)-1]=0;
}


int main(void)
{
    char big_string[] = "0123456789112345678921234567893123456789412345678951234567896123456789712345678981234567899123456789"
            "0123456789112345678921234567893123456789412345678951234567896123456789712345678981234567899123456789"
            "0123456789112345678921234567893123456789412345678951234567896123456789712345678981234567899123456789";

    PCP_LOGGER(PCP_DEBUG_NONE, "%s", "none");
    PCP_LOGGER(PCP_DEBUG_ERR, "%s", "error");
    PCP_LOGGER(PCP_DEBUG_WARN, "%s", "warn");
    PCP_LOGGER(PCP_DEBUG_INFO, "%s", "info");
    PCP_LOGGER(PCP_DEBUG_PERR, "%s", "perr");
    PCP_LOGGER(PCP_DEBUG_DEBUG, "%s", "debug");

    pcp_set_loggerfn(NULL);
    PCP_LOGGER(PCP_DEBUG_NONE, "big msg %s %s %s %s %s %s %s", big_string, big_string, big_string, big_string, big_string, big_string, big_string);

    pcp_set_loggerfn(test_logfn);
    PCP_LOGGER(PCP_DEBUG_NONE, "%s", "none");
    TEST((PCP_DEBUG_INFO<=PCP_MAX_LOG_LEVEL) ||((test_mode==PCP_DEBUG_NONE)&&(strcmp("none",test_msg)==0)));
    PCP_LOGGER(PCP_DEBUG_ERR, "%s", "error");
    TEST((PCP_DEBUG_INFO<=PCP_MAX_LOG_LEVEL) ||((test_mode==PCP_DEBUG_ERR)&&(strcmp("error",test_msg)==0)));
    PCP_LOGGER(PCP_DEBUG_WARN, "%s", "warn");
    TEST((PCP_DEBUG_INFO<=PCP_MAX_LOG_LEVEL) ||((test_mode==PCP_DEBUG_WARN)&&(strcmp("warn",test_msg)==0)));
    PCP_LOGGER(PCP_DEBUG_INFO, "%s", "info");
    TEST((PCP_DEBUG_INFO<=PCP_MAX_LOG_LEVEL) ||( (test_mode==PCP_DEBUG_INFO)&&(strcmp("info",test_msg)==0)));
    PCP_LOGGER(PCP_DEBUG_PERR, "%s", "perr");
    TEST((PCP_DEBUG_INFO<=PCP_MAX_LOG_LEVEL) ||((test_mode==PCP_DEBUG_PERR)&&(strcmp("perr",test_msg)==0)));
    PCP_LOGGER(PCP_DEBUG_DEBUG, "%s", "debug");
    TEST((PCP_DEBUG_INFO<=PCP_MAX_LOG_LEVEL) ||((test_mode==PCP_DEBUG_DEBUG)&&(strcmp("debug   ",test_msg)==0)));

    PCP_LOGGER(PCP_DEBUG_NONE, "big msg %s %s %s %s %s %s %s", big_string, big_string, big_string, big_string, big_string, big_string, big_string);
    TEST(strlen(test_msg)==2114);

    return 0;
}
