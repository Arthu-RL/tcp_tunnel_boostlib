#ifndef VARS_H
#define VARS_H

#pragma once

#define ADDR_BUF_SIZE 128
#define BUFF_SIZE 8192
#define MAX_CLIENTS 64

static char server_hostname[ADDR_BUF_SIZE] = "0.0.0.0";
static unsigned long int port = 25565;
static const char* protoname = "tcp";

#endif // VARS_H