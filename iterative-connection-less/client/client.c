/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless (UDP)
 * Client
 *
 * USAGE:
 *   ./client <server_ip>
 *   e.g.  ./client 192.168.1.105
 *
 * HOW THE CLIENT WORKS IN THIS MODEL:
 *   There is no connect() call. Each time the client needs
 *   something from the server it builds a command string,
 *   fires it as a single UDP datagram with sendto(), then
 *   calls recvfrom() and waits for the response datagram.
 *
 *   The client IS stateful (it tracks who is logged in locally
 *   in the `me` variable) but the SERVER is stateless — it does
 *   not remember the client between datagrams. Every request
 *   carries all the information the server needs (e.g. the
 *   username is always sent with MSG, LOGOUT, etc.).
 *
 *   CHAT MODEL (send-and-read-reply):
 *   In the iterative connectionless model there is no live
 *   push from the server. Instead the chat loop works as:
 *     1. You type a message
 *     2. Client sends MSG datagram, waits for ACK
 *     3. Client sends RECENT datagram to check for new messages
 *     4. Any new messages since last check are displayed
 *   This is a polling / request-response style chat.
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

static int                sock_fd;
static struct sockaddr_in server_addr;
static socklen_t          server_len;
static char               me[MAX_NAME_LEN + 1] = "";

static void clear_screen(void)           { printf("\033[2J\033[H"); }
static void print_error(const char *m)   { printf("  [!] %s\n", m); }
static void print_success(const char *m) { printf("  [+] %s\n", m); }
static void print_info(const char *m)    { printf("  [*] %s\n", m); }

static void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin)) buf[strcspn(buf, "\n")] = 0;
    else buf[0] = 0;
}

/*
 * send_recv — fire one datagram, wait for one response.
 * This is the only network function in the client.
 * Every operation is a single request/response cycle.
 */
static int send_recv(const char *msg, char *resp, int resp_size) {
    if (sendto(sock_fd, msg, strlen(msg), 0,
               (struct sockaddr *)&server_addr, server_len) < 0) {
        perror("  [!] sendto() failed");
        return -1;
    }

    int n = recvfrom(sock_fd, resp, resp_size - 1, 0, NULL, NULL);
    if (n < 0) { perror("  [!] recvfrom() failed"); return -1; }
    resp[n] = '\0';
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
    if (send_recv(cmd, resp, sizeof(resp)) < 0) return;

    if (strcmp(resp, CMD_ACK_OK) == 0)
        print_success("account created. you can now log in.");
    else
        print_error(resp + strlen(CMD_ACK_ERR) + 1);
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
    if (send_recv(cmd, resp, sizeof(resp)) < 0) return 0;

    if (strcmp(resp, CMD_ACK_OK) == 0) {
        strncpy(me, username, MAX_NAME_LEN);
        printf("\n  welcome, %s!\n", me);
        return 1;
    }
    print_error(resp + strlen(CMD_ACK_ERR) + 1);
    return 0;
}

/* ============================================================
 * FUNCTION : print_recent_messages
 * PURPOSE  : Request and display last N messages with partner.
 * ============================================================ */
static void print_recent_messages(const char *partner) {
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "%s:%s:%s", CMD_RECENT, me, partner);
    if (send_recv(cmd, resp, sizeof(resp)) < 0) return;
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
 * PURPOSE  : Send-and-read-reply style chat.
 *
 * NOTE ON THIS MODEL:
 *   Because the server is iterative and connectionless there is
 *   no live push. After you send a message you poll for recent
 *   messages so you can see what the other person wrote.
 *   Type /refresh to check for new messages from the other side.
 *   Type /quit to leave the chat.
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

        /* send the message */
        snprintf(cmd, sizeof(cmd), "%s:%s:%s:%s", CMD_MSG, me, partner, input);
        if (send_recv(cmd, resp, sizeof(resp)) < 0) { print_error("send failed."); break; }

        if (strcmp(resp, CMD_ACK_OK) == 0) {
            printf("  \033[33myou\033[0m: %s  \033[90m[sent]\033[0m\n", input);
        } else {
            print_error(resp + strlen(CMD_ACK_ERR) + 1);
        }
    }
    printf("\n");
}

/* ============================================================
 * SCREEN : screen_inbox
 * ============================================================ */
static void screen_inbox(void) {
    char cmd[BUFFER_SIZE], resp[BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "%s:%s", CMD_SENDERS, me);
    if (send_recv(cmd, resp, sizeof(resp)) < 0) { print_error("server error."); return; }

    clear_screen();
    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║  inbox — %-31s║\n", me);
    printf("  ╚══════════════════════════════════════════╝\n");

    if (strncmp(resp, "SENDERS_RESULT:", 15) != 0 || strlen(resp + 15) == 0) {
        printf("\n");
        print_info("inbox is empty.");
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

    if (send_recv(CMD_LIST, resp, sizeof(resp)) < 0) { print_error("server error."); return; }
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
    if (send_recv(cmd, resp, sizeof(resp)) < 0) { print_error("server error."); return; }

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
                if (send_recv(CMD_LIST, resp, sizeof(resp)) == 0 &&
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
                send_recv(cmd, resp, sizeof(resp));
                print_success("logged out.");
                me[0] = '\0';
                return;
            case 6: {
                char confirm[8];
                printf("  type YES to confirm: ");
                read_line(confirm, sizeof(confirm));
                if (strcmp(confirm, "YES") == 0) {
                    snprintf(cmd, sizeof(cmd), "%s:%s", CMD_DEREGISTER, me);
                    send_recv(cmd, resp, sizeof(resp));
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

    /* ── create UDP socket ── */
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) { perror("  [!] socket() failed"); exit(1); }

    /* ── configure server address from command-line IP ── */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    server_len                  = sizeof(server_addr);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "  [!] invalid server IP: %s\n", argv[1]);
        exit(1);
    }

    printf("\n  ╔══════════════════════════════════════════╗\n");
    printf("  ║   one-on-one chat  —  client             ║\n");
    printf("  ║   Assignment 3: Iterative Connectionless ║\n");
    printf("  ║   server: %-30s║\n", argv[1]);
    printf("  ╚══════════════════════════════════════════╝\n\n");

    int choice;
    while (1) {
        clear_screen();
        printf("\n  ╔══════════════════════════════════════════╗\n");
        printf("  ║      one-on-one chat  —  SCS3304         ║\n");
        printf("  ║      iterative connectionless (UDP)      ║\n");
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