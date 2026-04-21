/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connection-Oriented (TCP)
 * Server
 *
 * ════════════════════════════════════════════════════════════════
 * WHAT "ITERATIVE CONNECTION-ORIENTED" MEANS IN THIS FILE:
 *
 *   CONNECTION-ORIENTED:
 *     TCP requires a handshake before any data is exchanged.
 *     The server calls accept() which blocks until a client
 *     completes the TCP three-way handshake (SYN → SYN-ACK → ACK).
 *     Once accepted, a dedicated socket fd exists for that client
 *     and the server can send/recv on it as a reliable stream.
 *     When the session ends, close() tears the connection down.
 *     This is fundamentally different from UDP where there is no
 *     connection — just independent datagrams.
 *
 *   ITERATIVE:
 *     After accept(), the server enters client_session() and
 *     stays there until that client disconnects.
 *     There is NO fork(), NO pthread_create(), NO select().
 *     The server does not go back to accept() until the current
 *     client is completely done.
 *
 *     If a second client tries to connect while the first is
 *     being served, the OS will queue their SYN in the backlog
 *     (up to BACKLOG=5 pending connections). They will complete
 *     the TCP handshake and WAIT — but the server won't call
 *     accept() for them until the first client is finished.
 *
 *     The server log makes this crystal clear:
 *       [TCP-ITERATIVE] connection accepted from <IP> (fd=N)
 *       [TCP-ITERATIVE] serving client — all other connections QUEUED
 *       ...  (many commands handled here) ...
 *       [TCP-ITERATIVE] client disconnected. back to accept().
 *
 *   MULTI-MACHINE:
 *     Binds to 0.0.0.0 so it accepts connections from any machine.
 *     Client passes the server IP as a command-line argument.
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
#include "../common/utils.h"
#include "../common/user_manager.h"
#include "../common/message_handler.h"

/* ============================================================
 * FUNCTION : handle_command
 * PURPOSE  : Parse one command from the client and respond.
 *            Called inside client_session() for every message
 *            received over the TCP connection.
 * ============================================================ */
static void handle_command(int client_fd, char *cmd, char *session_user) {
    char response[BUFFER_SIZE];
    char command[16] = {0};
    char a1[50]      = {0};
    char a2[50]      = {0};
    char body[MAX_BODY_LEN] = {0};

    sscanf(cmd, "%15[^:]", command);

    /* ── REGISTER ── */
    if (strcmp(command, CMD_REGISTER) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^\n]", a1, a2);
        int r = register_user(a1, a2);
        if      (r == SUCCESS)        snprintf(response, sizeof(response), CMD_ACK_OK);
        else if (r == ERR_DUPLICATE)  snprintf(response, sizeof(response), "%s:username already taken", CMD_ACK_ERR);
        else if (r == ERR_INVALID)    snprintf(response, sizeof(response), "%s:invalid username or password too short", CMD_ACK_ERR);
        else                          snprintf(response, sizeof(response), "%s:server error", CMD_ACK_ERR);
        send_msg(client_fd, response);
    }

    /* ── LOGIN ── */
    else if (strcmp(command, CMD_LOGIN) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^\n]", a1, a2);
        int r = login_user(a1, a2);
        if (r == SUCCESS) {
            strncpy(session_user, a1, MAX_NAME_LEN);
            printf("  [TCP-ITERATIVE] %s logged in\n", a1);
            send_msg(client_fd, CMD_ACK_OK);
        } else if (r == ERR_NOT_FOUND)  { snprintf(response, sizeof(response), "%s:user not found",    CMD_ACK_ERR); send_msg(client_fd, response); }
        else if   (r == ERR_WRONG_PASS) { snprintf(response, sizeof(response), "%s:wrong password",    CMD_ACK_ERR); send_msg(client_fd, response); }
        else if   (r == ERR_ALREADY_ON) { snprintf(response, sizeof(response), "%s:already logged in", CMD_ACK_ERR); send_msg(client_fd, response); }
        else                            { snprintf(response, sizeof(response), "%s:login failed",       CMD_ACK_ERR); send_msg(client_fd, response); }
    }

    /* ── LOGOUT ── */
    else if (strcmp(command, CMD_LOGOUT) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        logout_user(a1);
        session_user[0] = '\0';
        printf("  [TCP-ITERATIVE] %s logged out\n", a1);
        send_msg(client_fd, CMD_ACK_OK);
    }

    /* ── DEREGISTER ── */
    else if (strcmp(command, CMD_DEREGISTER) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        logout_user(a1);
        int r = deregister_user(a1);
        session_user[0] = '\0';
        send_msg(client_fd, r == SUCCESS ? CMD_ACK_OK : CMD_ACK_ERR);
    }

    /* ── LIST ── */
    else if (strcmp(command, CMD_LIST) == 0) {
        build_user_list(response, sizeof(response));
        send_msg(client_fd, response);
    }

    /* ── SEARCH ── */
    else if (strcmp(command, CMD_SEARCH) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        build_search_result(a1, response, sizeof(response));
        send_msg(client_fd, response);
    }

    /* ── SEND MESSAGE ── */
    else if (strcmp(command, CMD_MSG) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^:]:%1023[^\n]", a1, a2, body);
        int r = store_message(a1, a2, body);
        if (r == SUCCESS) send_msg(client_fd, CMD_ACK_OK);
        else { snprintf(response, sizeof(response), "%s:recipient not found", CMD_ACK_ERR); send_msg(client_fd, response); }
    }

    /* ── INBOX SENDERS ── */
    else if (strcmp(command, CMD_SENDERS) == 0) {
        sscanf(cmd, "%*[^:]:%49[^\n]", a1);
        build_inbox_senders(a1, response, sizeof(response));
        send_msg(client_fd, response);
    }

    /* ── RECENT ── */
    else if (strcmp(command, CMD_RECENT) == 0) {
        sscanf(cmd, "%*[^:]:%49[^:]:%49[^\n]", a1, a2);
        build_recent_str(a1, a2, 8, response, sizeof(response));
        send_msg(client_fd, response);
    }

    /* ── unknown ── */
    else {
        snprintf(response, sizeof(response), "%s:unknown command", CMD_ACK_ERR);
        send_msg(client_fd, response);
    }
}

/* ============================================================
 * FUNCTION : client_session
 * PURPOSE  : Handle one client through their full session.
 *            The server stays in this function until the client
 *            disconnects — this is what makes it ITERATIVE.
 *            No other client can be accept()ed until this returns.
 * ============================================================ */
static void client_session(int client_fd, const char *client_ip) {
    char buf[BUFFER_SIZE];
    char session_user[MAX_NAME_LEN + 1] = "";

    printf("  [TCP-ITERATIVE] serving %s — all other connections QUEUED\n\n",
           client_ip);

    /*
     * Loop: recv one message, handle it, send response.
     * This loop holds the server exclusively until the
     * client closes the connection (recv_msg returns -1).
     */
    while (1) {
        if (recv_msg(client_fd, buf, sizeof(buf)) < 0) break;

        char cmd_word[16] = {0};
        sscanf(buf, "%15[^:]", cmd_word);
        printf("  [TCP-ITERATIVE]   cmd from %s: %s\n", client_ip, cmd_word);

        handle_command(client_fd, buf, session_user);
    }

    /* clean up on disconnect */
    if (session_user[0] != '\0') {
        logout_user(session_user);
        printf("  [TCP-ITERATIVE] %s disconnected (session ended)\n", session_user);
    }

    close(client_fd);

    /*
     * ── THIS IS THE KEY ITERATIVE MOMENT ──
     * We only reach this line after the client has fully
     * disconnected. The server now returns to main() and
     * calls accept() again. Only now can the next queued
     * client be served.
     */
    printf("  [TCP-ITERATIVE] client done. back to accept().\n");
    printf("  [TCP-ITERATIVE] ─────────────────────────────────────\n\n");
}

/* ============================================================
 * MAIN — the iterative accept loop
 * ============================================================ */
int main(void) {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;

    printf("\n  ╔══════════════════════════════════════════════╗\n");
    printf("  ║   one-on-one chat  —  server                 ║\n");
    printf("  ║   Assignment 3: Iterative Connection-Orient. ║\n");
    printf("  ║   Transport: TCP                             ║\n");
    printf("  ╚══════════════════════════════════════════════╝\n\n");

    /* ── create TCP socket ── */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("  [!] socket() failed"); exit(1); }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* ── bind to all interfaces ── */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("  [!] bind() failed"); exit(1);
    }

    /*
     * listen() with BACKLOG=5 means the OS will queue up to 5
     * pending TCP connections while the server is busy with the
     * current client. They complete the handshake and wait.
     * The server only calls accept() for them one at a time.
     */
    if (listen(server_fd, BACKLOG) < 0) {
        perror("  [!] listen() failed"); exit(1);
    }

    printf("  [*] TCP server bound to port %d\n", SERVER_PORT);
    printf("  [*] model: ITERATIVE CONNECTION-ORIENTED\n");
    printf("  [*] one client served at a time — others queue in backlog\n");
    printf("  [*] waiting for connections — Ctrl+C to stop\n\n");

    /*
     * ════════════════════════════════════════════════
     * THE ITERATIVE ACCEPT LOOP
     *
     * accept() blocks until a client connects.
     * We then call client_session() which handles that
     * client FULLY before returning here.
     * Only then do we call accept() again.
     * No fork, no threads, pure sequential processing.
     * ════════════════════════════════════════════════
     */
    while (1) {
        printf("  [TCP-ITERATIVE] waiting in accept()...\n");

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);
        if (client_fd < 0) { perror("  [!] accept() failed"); continue; }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        printf("  [TCP-ITERATIVE] connection accepted from %s (fd=%d)\n",
               client_ip, client_fd);

        /* serve this client — server is BLOCKED here until client disconnects */
        client_session(client_fd, client_ip);

        /* only reaches here after client_session() returns */
    }

    close(server_fd);
    return 0;
}