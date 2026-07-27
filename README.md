# ST5039CMD - Programming and Operating Systems

Softwarica College of IT & E-Commerce | Coventry University
March Intake 2026 | Coursework CW1

---

## Overview
This repository contains the implementation for ST5039CMD Programming and Operating Systems coursework. The assignment explores OS-level security mechanisms, process isolation, privilege management, and sandboxing using C programming on Linux.

---

## Task 1 - Privilege Separation in Password Validation

### How to Build
cd task1
make

### How to Run
Terminal 1 (Backend - needs root to drop privileges):
sudo ./backend

Terminal 2 (Frontend):
./frontend

### Test Credentials
Username: sugam
Password: secure123

### What it demonstrates
- Process isolation via UNIX domain sockets
- Privilege dropping from root to nobody (UID 65534) using setresuid()
- Secure memory clearing with explicit_bzero()
- Two independent processes communicating securely

---

## Task 2 - User Space Malware Analysis Sandbox

### How to Build
cd task2
make
gcc -o test_infinite test_infinite.c

### How to Run
./sandbox ./test_infinite

### What it demonstrates
- Parent-child process isolation via fork() and execve()
- 3 concurrent monitoring threads using pthreads
- Time limit enforcement using SIGKILL
- Resource monitoring via /proc filesystem
- All activity logged to sandbox.log

---

## Environment
- OS: Kali Linux
- Compiler: GCC
- Language: C (C11 standard)

---

## Author
Name: Sugam
Module: ST5039CMD Programming and Operating Systems
Intake: March 2026
