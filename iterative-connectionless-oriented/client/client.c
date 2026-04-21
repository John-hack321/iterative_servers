/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connection-Oriented (TCP)
 * Client
 *
 * USAGE:
 *   ./client <server_ip>
 *   e.g.  ./client 192.168.1.105
 *
 * HOW THIS DIFFERS FROM ASSIGNMENT 2:
 *   The app features are identical but the server behind it is
 *   iterative — it can only serve ONE client at a time. If you
 *   try to connect while another client is being served, your
 *   connection will be queued by the OS and you will see a
 *   short delay before the welcome menu appears (because connect()
 *   succeeds — the TCP handshake completes — but the server won't
 *   call accept() until the current session ends).
 *
 * CHAT MODEL:
 *   Send-and-read-reply. Type a message, it is sent and ACK'd.
 *   Use /refresh to poll for messages the other person sent.
 *   No live push — that requires concurrency on the server side.
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

static int  sock_fd = -1;
static char me[MAX_NAME_LEN + 1] = "";

static void clear_screen(void)           { printf("\033[2J\033[H"); }
static void print_error(const char *m)   { printf("  [!] %s\n", m); }
static void print_success(const char *m) { printf("  [+] %s\n", m); }
static void print_info(const char *m)    { printf("  [*] %s\n", m); }

static void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin)) buf[strcspn(buf, "\n")] = 0;
    else buf[0] = 0;
}

/* send one message over TCP, read one response */
static int exchange(const char *cmd, char *resp, int resp_size) {
    if (send_msg(sock_fd, cmd) < 0)             return -1;
    if (recv_msg(sock_fd, resp, resp_size) < 0) return -1;
    return 0;
}

/* ============================================================
 * FUNCTION : print_recent_messages
 * ============================================================ */
static void print_recent_messages(const char *partner) {
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "%s:%s:%s", CMD_RECENT, me, partner);
    if (exchange(cmd, resp, sizeof(resp)) < 0) return;
    if (strncmp(resp, "RECENT_RESULT:", 14) != 0) return;

    char *data = resp + 14;
    if (strlen(data) == 0) {
        print_info("no previous messages — start the conversation!");
        printf("\n");
        return;
    }

    printf("  ┌──────────────────────────────────────────────────────┐\n");
    printf("  │  last messages with %-32s│\n", partner);
    printf("  ├──────────────────────────────────────────────────────┤\n");

    char data_copy[BUFFER_SIZE];
    strncpy(data_copy, data, sizeof(data_copy) - 1);
    char *entry = strtok(data_copy, "~~");
    while (entry != NULL && strlen(entry) > 0) {
        char sender[50], body[MAX_BODY_LEN];
        if (sscanf(entry, "%49[^|]|%1023[^\n]", sender, body) == 2) {
            if (strcasecmp(sender, me) == 0)
                printf("  │  \033[33myou\033[0m: %s\n", body);
            else
                printf("  │  \033[36m%-10s\033[0m: %s\n", sender, body);
        }
        entry = strtok(NULL, "~~");
    }
    printf("  └──────────────────────────────────────────────────────┘\n\n");
}

/* ============================================================
 * FUNCTION : chat_loop
 * PURPOSE  : Send-and-read-reply chat over the persistent TCP
 *            connection.
 *
 * NOTE:
 *   The TCP connection stays open throughout this entire loop.
 *   Every send/recv happens on the same socket fd. This is
 *   what makes it connection-oriented — the server holds your
 *   socket open and no other client can connect until you leave.
 * ============================================================ */
static void chat_loop(const char *partner) {
    char input[MAX_BODY_LEN];
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];

    clear_screen();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║  chat: %-15s  ↔  %-12s║\n", me, partner);
    printf("  ║  /refresh to check new msgs              ║\n");
    printf("  ║  /quit    to leave                       ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    print_recent_messages(partner);

    while (1) {
        printf("  you: ");
        fflush(stdout);
        read_line(input, sizeof(input));

        if (strlen(input) == 0) continue;

        if (strcmp(input, "/quit") == 0) {
            print_info("left the chat.");
            break;
        }

        if (strcmp(input, "/refresh") == 0) {
            printf("\n");
            print_recent_messages(partner);
            continue;
        }

        snprintf(cmd, sizeof(cmd), "%s:%s:%s:%s", CMD_MSG, me, partner, input);
        if (exchange(cmd, resp, sizeof(resp)) < 0) { print_error("connection lost."); break; }

        if (strcmp(resp, CMD_ACK_OK) == 0)
            printf("  \033[33myou\033[0m: %s  \033[90m[sent]\033[0m\n", input);
        else
            print_error(resp + strlen(CMD_ACK_ERR) + 1);
    }
    printf("\n");
}

/* ============================================================
 * SCREEN : screen_inbox
 * ============================================================ */
static void screen_inbox(void) {
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "%s:%s", CMD_SENDERS, me);
    if (exchange(cmd, resp, sizeof(resp)) < 0) { print_error("server error."); return; }

    clear_screen();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║  inbox — %-31s║\n", me);
    printf("  ╚══════════════════════════════════════════╝\n");

    if (strncmp(resp, "SENDERS_RESULT:", 15) != 0 || strlen(resp + 15) == 0) {
        printf("\n"); print_info("inbox is empty.");
        printf("\n  press Enter to continue..."); getchar();
        return;
    }

    char *data = resp + 15;
    char  senders[50][50];
    int   count = 0;
    char  data_copy[BUFFER_SIZE];
    strncpy(data_copy, data, sizeof(data_copy) - 1);
    char *token = strtok(data_copy, "|");

    printf("\n  ┌────┬─────────────────────────────────────┐\n");
    printf("  │ #  │ conversation with                   │\n");
    printf("  ├────┼─────────────────────────────────────┤\n");

    while (token != NULL && count < 50 && strlen(token) > 0) {
        strncpy(senders[count], token, 49);
        senders[count][49] = '\0';
        printf("  │ %-2d │ %-35s │\n", count + 1, senders[count]);
        count++;
        token = strtok(NULL, "|");
    }

    if (count == 0) {
        printf("  │  no conversations yet.                 │\n");
        printf("  └────┴─────────────────────────────────────┘\n");
        printf("\n  press Enter to continue..."); getchar();
        return;
    }

    printf("  └────┴─────────────────────────────────────┘\n");
    printf("\n  enter number to open (0 to cancel): ");

    int pick;
    if (scanf("%d", &pick) != 1) { while(getchar()!='\n'); return; }
    while (getchar() != '\n');

    if (pick < 1 || pick > count) return;
    chat_loop(senders[pick - 1]);
}

/* ============================================================
 * SCREEN : screen_start_chat
 * ============================================================ */
static void screen_start_chat(void) {
    char resp[BUFFER_SIZE];

    if (exchange(CMD_LIST, resp, sizeof(resp)) < 0) { print_error("server error."); return; }
    if (strncmp(resp, "LIST_RESULT:", 12) != 0) return;

    clear_screen();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║  start chat — pick a user                ║\n");
    printf("  ╚══════════════════════════════════════════╝\n");

    char *data = resp + 12;
    char  names[MAX_USERS][50], statuses[MAX_USERS][10];
    int   count = 0;
    char  data_copy[BUFFER_SIZE];
    strncpy(data_copy, data, sizeof(data_copy) - 1);
    char *token = strtok(data_copy, "|");

    printf("\n  ┌────┬───────────────────┬──────────┐\n");
    printf("  │ #  │ username          │ status   │\n");
    printf("  ├────┼───────────────────┼──────────┤\n");

    while (token != NULL && count < MAX_USERS) {
        sscanf(token, "%49[^:]:%9s", names[count], statuses[count]);
        if (strcasecmp(names[count], me) != 0) {
            if (strcmp(statuses[count], STATUS_ONLINE) == 0)
                printf("  │ %-2d │ %-17s │ \033[32m%-8s\033[0m │\n",
                       count + 1, names[count], statuses[count]);
            else
                printf("  │ %-2d │ %-17s │ %-8s │\n",
                       count + 1, names[count], statuses[count]);
        }
        count++;
        token = strtok(NULL, "|");
    }
    printf("  └────┴───────────────────┴──────────┘\n");

    printf("\n  enter number (0 to cancel): ");
    int pick;
    if (scanf("%d", &pick) != 1) { while(getchar()!='\n'); return; }
    while (getchar() != '\n');

    if (pick < 1 || pick > count) return;
    if (strcasecmp(names[pick - 1], me) == 0) { print_error("cannot chat with yourself."); return; }
    chat_loop(names[pick - 1]);
}

/* ============================================================
 * SCREEN : screen_search
 * ============================================================ */
static void screen_search(void) {
    char target[MAX_NAME_LEN + 1], cmd[BUFFER_SIZE], resp[BUFFER_SIZE];

    clear_screen();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║  search user                             ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");
    printf("  username to search: "); read_line(target, sizeof(target));

    snprintf(cmd, sizeof(cmd), "%s:%s", CMD_SEARCH, target);
    if (exchange(cmd, resp, sizeof(resp)) < 0) { print_error("server error."); return; }

    if (strncmp(resp, "SEARCH_RESULT:", 14) == 0) {
        char *data = resp + 14;
        if (strcmp(data, "not_found") == 0) { print_error("user not found."); return; }
        char name[50], status[10];
        sscanf(data, "%49[^:]:%9s", name, status);
        printf("  ┌─────────────────────────────┐\n");
        printf("  │ username : %-16s│\n", name);
        printf("  │ status   : %-16s│\n", status);
        printf("  └─────────────────────────────┘\n");
    }
}

/* ============================================================
 * SCREEN : screen_login
 * ============================================================ */
static int screen_login(void) {
    char username[MAX_NAME_LEN + 1], password[64];
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];

    clear_screen();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║  login                                   ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  username : "); read_line(username, sizeof(username));
    printf("  password : "); read_line(password, sizeof(password));

    snprintf(cmd, sizeof(cmd), "%s:%s:%s", CMD_LOGIN, username, password);
    if (exchange(cmd, resp, sizeof(resp)) < 0) { print_error("server error."); return 0; }

    if (strcmp(resp, CMD_ACK_OK) == 0) {
        strncpy(me, username, MAX_NAME_LEN);
        printf("\n  welcome, %s!\n", me);
        return 1;
    }
    print_error(resp + strlen(CMD_ACK_ERR) + 1);
    return 0;
}

/* ============================================================
 * SCREEN : screen_register
 * ============================================================ */
static void screen_register(void) {
    char username[MAX_NAME_LEN + 1], password[64], confirm[64];
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];

    clear_screen();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║  register new account                    ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");

    printf("  username (no spaces)   : "); read_line(username, sizeof(username));
    printf("  password (min 4 chars) : "); read_line(password, sizeof(password));
    printf("  confirm password       : "); read_line(confirm,  sizeof(confirm));

    if (strcmp(password, confirm) != 0) { print_error("passwords do not match."); return; }

    snprintf(cmd, sizeof(cmd), "%s:%s:%s", CMD_REGISTER, username, password);
    if (exchange(cmd, resp, sizeof(resp)) < 0) { print_error("server error."); return; }

    if (strcmp(resp, CMD_ACK_OK) == 0)
        print_success("account created. you can now log in.");
    else
        print_error(resp + strlen(CMD_ACK_ERR) + 1);
}

/* ============================================================
 * MENU : logged_in_menu
 * ============================================================ */
static void logged_in_menu(void) {
    int choice;
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];

    while (1) {
        clear_screen();
        printf("\n  ╔══════════════════════════════════════════╗\n");
        printf("  ║  logged in as: %-26s║\n", me);
        printf("  ╠══════════════════════════════════════════╣\n");
        printf("  ║  1.  inbox  (reply to messages)          ║\n");
        printf("  ║  2.  start new chat                      ║\n");
        printf("  ║  3.  search user                         ║\n");
        printf("  ║  4.  list all users                      ║\n");
        printf("  ║  5.  logout                              ║\n");
        printf("  ║  6.  delete account                      ║\n");
        printf("  ╚══════════════════════════════════════════╝\n");
        printf("  choice: ");

        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); continue; }
        while (getchar() != '\n');

        switch (choice) {
            case 1: screen_inbox();      break;
            case 2: screen_start_chat(); break;
            case 3: screen_search();     break;
            case 4:
                clear_screen();
                if (exchange(CMD_LIST, resp, sizeof(resp)) == 0 &&
                    strncmp(resp, "LIST_RESULT:", 12) == 0) {
                    char dc[BUFFER_SIZE];
                    strncpy(dc, resp + 12, sizeof(dc) - 1);
                    char names[MAX_USERS][50], statuses[MAX_USERS][10];
                    int  n = 0;
                    char *t = strtok(dc, "|");
                    printf("\n  ┌────┬───────────────────┬──────────┐\n");
                    printf("  │ #  │ username          │ status   │\n");
                    printf("  ├────┼───────────────────┼──────────┤\n");
                    while (t && n < MAX_USERS) {
                        sscanf(t, "%49[^:]:%9s", names[n], statuses[n]);
                        if (strcmp(statuses[n], STATUS_ONLINE) == 0)
                            printf("  │ %-2d │ %-17s │ \033[32m%-8s\033[0m │\n", n+1, names[n], statuses[n]);
                        else
                            printf("  │ %-2d │ %-17s │ %-8s │\n", n+1, names[n], statuses[n]);
                        n++; t = strtok(NULL, "|");
                    }
                    printf("  └────┴───────────────────┴──────────┘\n");
                }
                break;
            case 5:
                snprintf(cmd, sizeof(cmd), "%s:%s", CMD_LOGOUT, me);
                exchange(cmd, resp, sizeof(resp));
                print_success("logged out.");
                me[0] = '\0';
                return;
            case 6: {
                char confirm[8];
                printf("  type YES to confirm: ");
                read_line(confirm, sizeof(confirm));
                if (strcmp(confirm, "YES") == 0) {
                    snprintf(cmd, sizeof(cmd), "%s:%s", CMD_DEREGISTER, me);
                    exchange(cmd, resp, sizeof(resp));
                    print_success("account deleted.");
                    me[0] = '\0';
                    return;
                }
                break;
            }
            default: print_error("invalid choice.");
        }
        printf("\n  press Enter to continue..."); getchar();
    }
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "  usage: %s <server_ip>\n", argv[0]);
        fprintf(stderr, "  e.g.:  %s 192.168.1.105\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in server_addr;

    /* ── create TCP socket ── */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) { perror("  [!] socket() failed"); exit(1); }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "  [!] invalid server IP: %s\n", argv[1]);
        exit(1);
    }

    printf("\n  connecting to server at %s:%d ...\n", argv[1], SERVER_PORT);

    /*
     * connect() does the TCP three-way handshake.
     * If the server is already serving another client this call
     * may block for a moment (until the server calls accept()).
     * Once connected, this socket stays open for the whole session.
     */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("  [!] connect() failed — is the server running?");
        exit(1);
    }

    printf("  [+] TCP connection established.\n\n");

    printf("\n  ╔══════════════════════════════════════════════╗\n");
    printf("  ║   one-on-one chat  —  client                 ║\n");
    printf("  ║   Assignment 3: Iterative Connection-Orient. ║\n");
    printf("  ║   server: %-32s║\n", argv[1]);
    printf("  ╚══════════════════════════════════════════════╝\n\n");

    int choice;
    while (1) {
        clear_screen();
        printf("\n  ╔══════════════════════════════════════════╗\n");
        printf("  ║      one-on-one chat  —  SCS3304         ║\n");
        printf("  ║      iterative connection-oriented (TCP) ║\n");
        printf("  ╠══════════════════════════════════════════╣\n");
        printf("  ║  1.  login                               ║\n");
        printf("  ║  2.  register new account                ║\n");
        printf("  ║  3.  exit                                ║\n");
        printf("  ╚══════════════════════════════════════════╝\n");
        printf("  choice: ");

        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); continue; }
        while (getchar() != '\n');

        if      (choice == 1) { if (screen_login()) logged_in_menu(); else { printf("\n  press Enter to try again..."); getchar(); } }
        else if (choice == 2) { screen_register(); printf("\n  press Enter to continue..."); getchar(); }
        else if (choice == 3) { printf("  goodbye.\n\n"); close(sock_fd); exit(0); }
    }
}