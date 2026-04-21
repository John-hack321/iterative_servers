/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless (UDP)
 * Protocol Header
 *
 * WHY UDP / CONNECTIONLESS?
 *   In this model there is no handshake, no persistent connection,
 *   and no guarantee of delivery or ordering. The client fires a
 *   datagram at the server and waits for one back. The server never
 *   "knows" a client — it just sees packets arrive from some address.
 *
 *   This is connectionless: each request/response pair is independent.
 *   This is iterative: the server handles one datagram fully before
 *   reading the next one.
 *
 * MESSAGE FORMAT:
 *   Every datagram is a plain text string:
 *     COMMAND:arg1:arg2:...
 *   Max size is BUFFER_SIZE. No length prefix needed — UDP preserves
 *   datagram boundaries automatically (unlike TCP streams).
 *
 * MULTI-MACHINE SETUP:
 *   SERVER_PORT is the only thing both sides need to agree on.
 *   The client takes the server IP as a command-line argument so
 *   it works across different machines on the same network.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

/* ── network ── */
#define SERVER_PORT   9090          /* UDP port the server binds to     */
#define BUFFER_SIZE   2048          /* max datagram size                 */

/* ── commands (client → server) ── */
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

/* ── responses (server → client) ── */
#define CMD_ACK_OK     "ACK:OK"
#define CMD_ACK_ERR    "ACK:ERR"

#endif