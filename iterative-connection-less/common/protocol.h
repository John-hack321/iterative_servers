#ifndef PROTOCOL_H
#define PROTOCOL_H

/* ── network ── */
#define SERVER_PORT   9090          /* UDP port the server binds to     */
#define BUFFER_SIZE   2048          /* max datagram size                 */

/* ── commands (client to server) ── */
#define CMD_REGISTER   "REG"        /* REG:username:password             */
#define CMD_LOGIN      "LOGIN"      /* LOGIN:username:password           */
#define CMD_LOGOUT     "LOGOUT"     /* LOGOUT:username                   */
#define CMD_DEREGISTER "DEREG"      /* DEREG:username                    */
#define CMD_LIST       "LIST"       /* LIST                              */
#define CMD_SEARCH     "SEARCH"     /* SEARCH:username                   */
#define CMD_MSG        "MSG"        /* MSG:from:to:body                  */
#define CMD_INBOX      "INBOX"      /* INBOX:username                    */
#define CMD_SENDERS    "SENDERS"    /* SENDERS:username                  */
#define CMD_RECENT     "RECENT"     /* RECENT:user_a:user_b              */

/* ── responses (server to client) ── */
#define CMD_ACK_OK     "ACK:OK"
#define CMD_ACK_ERR    "ACK:ERR"

#endif