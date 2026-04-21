/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless (UDP)
 * Server
 *
 * ════════════════════════════════════════════════════════════════
 * WHAT "ITERATIVE CONNECTIONLESS" MEANS IN THIS FILE:
 *
 *   ITERATIVE:
 *     The server has exactly ONE socket and ONE loop.
 *     It calls recvfrom(), handles the request completely,
 *     calls sendto() with the response, and ONLY THEN goes
 *     back to recvfrom() to wait for the next datagram.
 *     There is no fork(), no threads, no select(). While the
 *     server is processing one request, every other client
 *     that sends a datagram must WAIT. They are queued by the
 *     OS in the socket's receive buffer until the server is
 *     ready. This is what makes it iterative.
 *
 *   CONNECTIONLESS:
 *     UDP has no handshake. The server never calls accept().
 *     There is no "connection" object. The server just sees
 *     raw datagrams arrive with a source (IP, port) attached.
 *     Each datagram is completely independent — the server
 *     doesn't remember the client between requests. Even the
 *     client's login state must be tracked explicitly in the
 *     data files (users.txt) because there is no socket-level
 *     session to hold that state.
 *
 *   MULTI-MACHINE:
 *     The server binds to 0.0.0.0 (all interfaces) so it
 *     accepts datagrams from any machine on the network.
 *     The client takes the server's IP as a command-line arg.
 *
 * SERVER PROOF OUTPUT:
 *     Every time a datagram arrives you will see:
 *       [UDP-ITERATIVE] datagram from <IP>:<port> — cmd: <CMD>
 *       [UDP-ITERATIVE] request handled. waiting for next datagram...
 *     This makes it visually clear that the server handles one
 *     request at a time before looping back to wait.
 * ════════════════════════════════════════════════════════════════
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../common/protocol.h"
#include "../common/user_manager.h"
#include "../common/message_handler.h"

/* ============================================================
 * FUNCTION : handle_datagram
 * PURPOSE  : Parse one UDP datagram and build the response.
 *            Called once per recvfrom() — fully synchronous.
 * ============================================================ */
static void handle_datagram(const char *cmd, char *response, int resp_size) {
    char command[16] = {0};
    char a1[50]      = {0};
    char a2[50]      = {0};
    char body[MAX_BODY_LEN] = {0};

    sscanf(cmd, "%15[^:]", command);

    /* ── REGISTER ── REG:username:password ── */
    if (strcmp(command, CMD_REGISTER) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^\n]", a1, a2);
        int r = register_user(a1, a2);
        if      (r == SUCCESS)       snprintf(response, resp_size, CMD_ACK_OK);
        else if (r == ERR_DUPLICATE) snprintf(response, resp_size, "%s:username already taken", CMD_ACK_ERR);
        else if (r == ERR_INVALID)   snprintf(response, resp_size, "%s:invalid username or password too short", CMD_ACK_ERR);
        else                         snprintf(response, resp_size, "%s:server error", CMD_ACK_ERR);
    }

    /* ── LOGIN ── LOGIN:username:password ── */
    else if (strcmp(command, CMD_LOGIN) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^\n]", a1, a2);
        int r = login_user(a1, a2);
        if      (r == SUCCESS)       snprintf(response, resp_size, CMD_ACK_OK);
        else if (r == ERR_NOT_FOUND) snprintf(response, resp_size, "%s:user not found",    CMD_ACK_ERR);
        else if (r == ERR_WRONG_PASS)snprintf(response, resp_size, "%s:wrong password",    CMD_ACK_ERR);
        else if (r == ERR_ALREADY_ON)snprintf(response, resp_size, "%s:already logged in", CMD_ACK_ERR);
        else                         snprintf(response, resp_size, "%s:login failed",       CMD_ACK_ERR);
    }

    /* ── LOGOUT ── LOGOUT:username ── */
    else if (strcmp(command, CMD_LOGOUT) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        logout_user(a1);
        snprintf(response, resp_size, CMD_ACK_OK);
    }

    /* ── DEREGISTER ── DEREG:username ── */
    else if (strcmp(command, CMD_DEREGISTER) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        logout_user(a1);
        int r = deregister_user(a1);
        snprintf(response, resp_size, r == SUCCESS ? CMD_ACK_OK : CMD_ACK_ERR);
    }

    /* ── LIST ── */
    else if (strcmp(command, CMD_LIST) == 0) {
        build_user_list(response, resp_size);
    }

    /* ── SEARCH ── SEARCH:username ── */
    else if (strcmp(command, CMD_SEARCH) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        build_search_result(a1, response, resp_size);
    }

    /* ── SEND MESSAGE ── MSG:from:to:body ── */
    else if (strcmp(command, CMD_MSG) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^:]:%1023[^\n]", a1, a2, body);
        int r = store_message(a1, a2, body);
        if (r == SUCCESS) snprintf(response, resp_size, CMD_ACK_OK);
        else              snprintf(response, resp_size, "%s:recipient not found", CMD_ACK_ERR);
    }

    /* ── INBOX SENDERS ── SENDERS:username ── */
    else if (strcmp(command, CMD_SENDERS) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        build_inbox_senders(a1, response, resp_size);
    }

    /* ── RECENT ── RECENT:user_a:user_b ── */
    else if (strcmp(command, CMD_RECENT) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^\n]", a1, a2);
        build_recent_str(a1, a2, 8, response, resp_size);
    }

    /* ── unknown ── */
    else {
        snprintf(response, resp_size, "%s:unknown command", CMD_ACK_ERR);
    }
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    int sock_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buf[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║   one-on-one chat  —  server             ║\n");
    printf("  ║   Assignment 3: Iterative Connectionless ║\n");
    printf("  ║   Transport: UDP                         ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    /* ── create UDP socket ── */
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) { perror("  [!] socket() failed"); exit(1); }

    /* ── bind to all interfaces on SERVER_PORT ── */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;   /* accept from any machine */

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("  [!] bind() failed");
        exit(1);
    }

    printf("  [*] UDP server bound to port %d\n", SERVER_PORT);
    printf("  [*] model: ITERATIVE CONNECTIONLESS\n");
    printf("  [*] waiting for datagrams — Ctrl+C to stop\n\n");

    /*
     * ════════════════════════════════════════════════
     * THE ITERATIVE LOOP
     *
     * recvfrom() blocks until a datagram arrives.
     * We handle it COMPLETELY — including writing to
     * files and building the response — before calling
     * recvfrom() again. This is what makes it iterative:
     * one request at a time, no concurrency at all.
     * ════════════════════════════════════════════════
     */
    while (1) {
        memset(buf,      0, sizeof(buf));
        memset(response, 0, sizeof(response));

        /* BLOCK here until a datagram arrives */
        int n = recvfrom(sock_fd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) { perror("  [!] recvfrom() error"); continue; }

        buf[n] = '\0';

        /* extract command word for the log */
        char cmd_word[16] = {0};
        sscanf(buf, "%15[^:]", cmd_word);

        printf("  [UDP-ITERATIVE] datagram from %s:%d — cmd: %s\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               cmd_word);

        /* handle the request — FULLY synchronous, no fork */
        handle_datagram(buf, response, sizeof(response));

        /* send response back to the exact client that sent this datagram */
        sendto(sock_fd, response, strlen(response), 0,
               (struct sockaddr *)&client_addr, client_len);

        printf("  [UDP-ITERATIVE] response sent. waiting for next datagram...\n\n");
        /* loop back — next client must wait until we reach recvfrom() again */
    }

    close(sock_fd);
    return 0;
}