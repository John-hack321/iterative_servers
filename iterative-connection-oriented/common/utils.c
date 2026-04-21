/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connection-Oriented (TCP)
 * Network Framing Utilities
 *
 * Every message is sent as:
 *   [4-byte length (network byte order)][message body]
 *
 * htonl / ntohl convert between host and network byte order so
 * the two machines (which may have different CPU architectures)
 * always agree on the integer value.
 */

#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"

int send_msg(int fd, const char *msg) {
    uint32_t len     = (uint32_t)strlen(msg);
    uint32_t net_len = htonl(len);

    if (send(fd, &net_len, sizeof(net_len), 0) != sizeof(net_len)) return -1;
    if (send(fd, msg, len, 0) != (ssize_t)len)                     return -1;
    return 0;
}

int recv_msg(int fd, char *buf, int buf_size) {
    uint32_t net_len;
    if (recv(fd, &net_len, sizeof(net_len), MSG_WAITALL) <= 0) return -1;

    uint32_t len = ntohl(net_len);
    if ((int)len >= buf_size) len = (uint32_t)(buf_size - 1);

    if (recv(fd, buf, len, MSG_WAITALL) <= 0) return -1;
    buf[len] = '\0';
    return 0;
}