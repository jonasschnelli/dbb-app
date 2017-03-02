// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_APP_H
#define DBB_APP_H

#ifndef _SRC_CONFIG__DBB_CONFIG_H
#include "config/_dbb-config.h"
#endif

typedef enum DBB_CMD_EXECUTION_STATUS
{
    DBB_CMD_EXECUTION_STATUS_OK,
    DBB_CMD_EXECUTION_STATUS_ENCRYPTION_FAILED,
    DBB_CMD_EXECUTION_DEVICE_OPEN_FAILED,
} dbb_cmd_execution_status_t;

#endif
