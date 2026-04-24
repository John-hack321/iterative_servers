/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless 
 * Authentication Module
 *
 * djb2 hash for password hashing
 */

#include <stdio.h>
#include <string.h>
#include "auth.h"

unsigned int hash_password(const char *password) {
    unsigned int hash = 5381;
    int c;
    while ((c = (unsigned char)*password++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

int verify_password(const char *plain, const char *stored) {
    char computed[16];
    snprintf(computed, sizeof(computed), "%u", hash_password(plain));
    return (strcmp(computed, stored) == 0);
}