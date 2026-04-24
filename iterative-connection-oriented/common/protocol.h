#ifndef PROTOCOL_H
#define PROTOCOL_H

/* ── network ── */
#define SERVER_PORT  9091           /* TCP port — different from UDP impl */
#define BACKLOG      5              /* OS connection queue length          */
#define BUFFER_SIZE  2048

/* ── commands (client → server) ── */
#define CMD_REGISTER   "REG"
#define CMD_LOGIN      "LOGIN"
#define CMD_LOGOUT     "LOGOUT"
#define CMD_DEREGISTER "DEREG"
#define CMD_LIST       "LIST"
#define CMD_SEARCH     "SEARCH"
#define CMD_MSG        "MSG"
#define CMD_SENDERS    "SENDERS"
#define CMD_RECENT     "RECENT"

/* ── responses (server → client) ── */
#define CMD_ACK_OK  "ACK:OK"
#define CMD_ACK_ERR "ACK:ERR"

#endif