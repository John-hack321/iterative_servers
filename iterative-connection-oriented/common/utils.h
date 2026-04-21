/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connection-Oriented (TCP)
 * Network Framing Utilities Header
 *
 * TCP is a byte stream — it does not preserve message boundaries.
 * send_msg / recv_msg add a 4-byte length prefix to every message
 * so the receiver always knows exactly how many bytes to read.
 */

#ifndef UTILS_H
#define UTILS_H

int send_msg(int fd, const char *msg);
int recv_msg(int fd, char *buf, int buf_size);

#endif