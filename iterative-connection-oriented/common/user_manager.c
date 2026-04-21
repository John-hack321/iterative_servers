/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless (UDP)
 * User Manager Implementation
 *
 * FILE LOCKING NOTE:
 *   Even though the server is iterative (one request at a time),
 *   we keep flock() on all file operations. This is good practice
 *   and ensures the data files are safe if the server is ever
 *   upgraded to a concurrent model later.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/file.h>
#include "user_manager.h"
#include "auth.h"

static void now(char *buf, int size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M", tm_info);
}

static void parse_line(const char *line, User *u) {
    memset(u, 0, sizeof(User));
    sscanf(line, "%49[^:]:%15[^:]:%9[^:]:%19[^\n]",
           u->username, u->password_hash, u->status, u->last_seen);
}

static int load_users(char lines[MAX_USERS][128]) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) return 0;
    flock(fileno(fp), LOCK_SH);
    int count = 0;
    while (count < MAX_USERS && fgets(lines[count], 128, fp) != NULL)
        count++;
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return count;
}

static int save_lines(char lines[MAX_USERS][128], int count) {
    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) return ERR_FILE;
    flock(fileno(fp), LOCK_EX);
    for (int i = 0; i < count; i++)
        fputs(lines[i], fp);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return SUCCESS;
}

int user_exists(const char *username) {
    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    User u;
    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        if (strcasecmp(u.username, username) == 0) return 1;
    }
    return 0;
}

int is_online(const char *username) {
    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    User u;
    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        if (strcasecmp(u.username, username) == 0)
            return (strcmp(u.status, STATUS_ONLINE) == 0);
    }
    return 0;
}

int register_user(const char *username, const char *password) {
    if (strlen(username) == 0 || strlen(password) < MIN_PASSWORD_LEN)
        return ERR_INVALID;

    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    if (count >= MAX_USERS) return ERR_FULL;

    User u;
    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        if (strcasecmp(u.username, username) == 0) return ERR_DUPLICATE;
    }

    char hash_str[16], timestamp[MAX_TIME_LEN];
    snprintf(hash_str, sizeof(hash_str), "%u", hash_password(password));
    now(timestamp, sizeof(timestamp));

    FILE *fp = fopen(USERS_FILE, "a");
    if (fp == NULL) return ERR_FILE;
    flock(fileno(fp), LOCK_EX);
    fprintf(fp, "%s:%s:%s:%s\n", username, hash_str, STATUS_OFFLINE, timestamp);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return SUCCESS;
}

int login_user(const char *username, const char *password) {
    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    int  found = -1;
    User u;

    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        if (strcasecmp(u.username, username) == 0) { found = i; break; }
    }
    if (found == -1) return ERR_NOT_FOUND;
    if (!verify_password(password, u.password_hash)) return ERR_WRONG_PASS;
    if (strcmp(u.status, STATUS_ONLINE) == 0) return ERR_ALREADY_ON;

    char timestamp[MAX_TIME_LEN];
    now(timestamp, sizeof(timestamp));
    snprintf(lines[found], 128, "%s:%s:%s:%s\n",
             u.username, u.password_hash, STATUS_ONLINE, timestamp);
    return save_lines(lines, count);
}

int logout_user(const char *username) {
    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    int  found = -1;
    User u;

    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        if (strcasecmp(u.username, username) == 0) { found = i; break; }
    }
    if (found == -1) return ERR_NOT_FOUND;

    char timestamp[MAX_TIME_LEN];
    now(timestamp, sizeof(timestamp));
    snprintf(lines[found], 128, "%s:%s:%s:%s\n",
             u.username, u.password_hash, STATUS_OFFLINE, timestamp);
    return save_lines(lines, count);
}

int deregister_user(const char *username) {
    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    int  found = 0;
    User u;

    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) return ERR_FILE;
    flock(fileno(fp), LOCK_EX);
    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        if (strcasecmp(u.username, username) == 0) { found = 1; continue; }
        fputs(lines[i], fp);
    }
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return found ? SUCCESS : ERR_NOT_FOUND;
}

void build_user_list(char *buf, int buf_size) {
    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    User u;

    snprintf(buf, buf_size, "LIST_RESULT:");
    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        strncat(buf, u.username, buf_size - strlen(buf) - 1);
        strncat(buf, ":",        buf_size - strlen(buf) - 1);
        strncat(buf, u.status,   buf_size - strlen(buf) - 1);
        strncat(buf, "|",        buf_size - strlen(buf) - 1);
    }
}

void build_search_result(const char *username, char *buf, int buf_size) {
    char lines[MAX_USERS][128];
    int  count = load_users(lines);
    User u;

    for (int i = 0; i < count; i++) {
        parse_line(lines[i], &u);
        if (strcasecmp(u.username, username) == 0) {
            snprintf(buf, buf_size, "SEARCH_RESULT:%s:%s",
                     u.username,
                     strcmp(u.status, STATUS_ONLINE) == 0 ? STATUS_ONLINE : STATUS_OFFLINE);
            return;
        }
    }
    snprintf(buf, buf_size, "SEARCH_RESULT:not_found");
}