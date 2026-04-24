/*
 * SCS3304 — One-on-One Chat Application
 * Assignment 3 — Iterative Connectionless
 * Authentication Module Header
 */

#ifndef AUTH_H
#define AUTH_H

#define MIN_PASSWORD_LEN 4

unsigned int hash_password(const char *password);
int          verify_password(const char *plain, const char *stored);

#endif