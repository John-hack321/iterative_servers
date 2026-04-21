/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless (UDP)
 * Message Handler Implementation
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/file.h>
#include "message_handler.h"
#include "user_manager.h"

static void now(char *buf, int size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

int store_message(const char *from, const char *to, const char *body) {
    if (!user_exists(to)) return ERR_NOT_FOUND;

    char timestamp[24];
    now(timestamp, sizeof(timestamp));

    FILE *fp = fopen(MESSAGES_FILE, "a");
    if (fp != NULL) {
        flock(fileno(fp), LOCK_EX);
        fprintf(fp, "%s|%s|%s|%s\n", timestamp, from, to, body);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
    }

    fp = fopen(LOG_FILE, "a");
    if (fp != NULL) {
        flock(fileno(fp), LOCK_EX);
        fprintf(fp, "[%s] %s -> %s : %s\n", timestamp, from, to, body);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
    }

    return SUCCESS;
}

void build_inbox_senders(const char *username, char *buf, int buf_size) {
    snprintf(buf, buf_size, "SENDERS_RESULT:");

    FILE *fp = fopen(MESSAGES_FILE, "r");
    if (fp == NULL) return;

    flock(fileno(fp), LOCK_SH);

    char unique[50][50];
    int  ucount = 0;
    char line[MAX_BODY_LEN + 128];
    char ts[24], from[50], to[50], body[MAX_BODY_LEN];

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "%23[^|]|%49[^|]|%49[^|]|%1023[^\n]",
                   ts, from, to, body) == 4) {
            if (strcasecmp(to, username) == 0) {
                int found = 0;
                for (int i = 0; i < ucount; i++) {
                    if (strcasecmp(unique[i], from) == 0) {
                        found = 1;
                        memmove(&unique[i], &unique[i+1],
                                (ucount - i - 1) * sizeof(unique[0]));
                        strncpy(unique[ucount - 1], from, 49);
                        break;
                    }
                }
                if (!found && ucount < 50) {
                    strncpy(unique[ucount], from, 49);
                    unique[ucount][49] = '\0';
                    ucount++;
                }
            }
        }
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    for (int i = ucount - 1; i >= 0; i--) {
        strncat(buf, unique[i], buf_size - strlen(buf) - 1);
        strncat(buf, "|",       buf_size - strlen(buf) - 1);
    }
}

void build_recent_str(const char *user_a, const char *user_b,
                      int limit, char *buf, int buf_size) {
    snprintf(buf, buf_size, "RECENT_RESULT:");

    FILE *fp = fopen(MESSAGES_FILE, "r");
    if (fp == NULL) return;

    flock(fileno(fp), LOCK_SH);

    char senders[20][50];
    char bodies[20][MAX_BODY_LEN];
    int  total = 0;
    char line[MAX_BODY_LEN + 128];
    char ts[24], from[50], to[50], body[MAX_BODY_LEN];

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "%23[^|]|%49[^|]|%49[^|]|%1023[^\n]",
                   ts, from, to, body) == 4) {
            int a_b = strcasecmp(from,user_a)==0 && strcasecmp(to,user_b)==0;
            int b_a = strcasecmp(from,user_b)==0 && strcasecmp(to,user_a)==0;
            if (a_b || b_a) {
                int slot = total % limit;
                strncpy(senders[slot], from,  49);
                strncpy(bodies[slot],  body, MAX_BODY_LEN - 1);
                senders[slot][49] = '\0';
                bodies[slot][MAX_BODY_LEN - 1] = '\0';
                total++;
            }
        }
    }

    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    if (total == 0) return;

    int count = total < limit ? total : limit;
    int start = total < limit ? 0 : (total % limit);

    for (int i = 0; i < count; i++) {
        int slot = (start + i) % limit;
        char entry[MAX_BODY_LEN + 64];
        snprintf(entry, sizeof(entry), "%s|%s~~", senders[slot], bodies[slot]);
        strncat(buf, entry, buf_size - strlen(buf) - 1);
    }
}