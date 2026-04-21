/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless (UDP)
 * Message Handler Header
 */

#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#define MESSAGES_FILE "data/messages.txt"
#define LOG_FILE      "data/chat_log.txt"
#define MAX_BODY_LEN  1024

int  store_message(const char *from, const char *to, const char *body);
void build_inbox_senders(const char *username, char *buf, int buf_size);
void build_recent_str(const char *user_a, const char *user_b, int limit, char *buf, int buf_size);

#endif