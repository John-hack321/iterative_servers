/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connection-Oriented (TCP)
 * Protocol Header
 *
 * WHY TCP / CONNECTION-ORIENTED?
 *   TCP establishes a persistent connection via a three-way
 *   handshake (SYN, SYN-ACK, ACK) before any data is exchanged.
 *   The server explicitly accept()s the connection, holds it
 *   open for the duration of the client session, then close()s
 *   it when the client disconnects. The connection has state.
 *
 * WHY ITERATIVE?
 *   The server handles ONE client at a time. After accept()ing
 *   a connection, the server serves that client through their
 *   entire session (login → chat → logout). Only when that
 *   client disconnects does the server go back to accept().
 *   Any other client trying to connect during this time will
 *   be queued in the OS backlog — they cannot be served until
 *   the current session ends.
 *
 *   This is the key difference from Assignment 2's server,
 *   which used fork() to handle multiple clients simultaneously.
 *   Here: NO fork(), NO threads — pure iterative.
 *
 * MESSAGE FORMAT:
 *   [4-byte length header (network byte order)][message body]
 *   TCP is a stream — without framing, recv() has no way to
 *   know where one message ends. The length prefix solves this.
 *
 * MULTI-MACHINE SETUP:
 *   Server binds to 0.0.0.0 (all interfaces).
 *   Client takes server IP as a command-line argument.
 */

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