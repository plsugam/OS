# ST5039CMD - Programming and Operating Systems

Softwarica College of IT & E-Commerce | Coventry University

## Overview
This repository contains the implementation and documentation for ST5039CMD Programming and Operating Systems coursework. The assignment explores OS-level security mechanisms, process isolation, privilege management, and sandboxing using C programming on Linux.

## Repository Structure

- task1/ - Privilege Separation in Password Validation
  - frontend.c - User input handler process
  - backend.c - Privileged validation process
  - Makefile - Build instructions

- task2/ - User Space Malware Analysis Sandbox
  - sandbox.c - Sandbox controller
  - Makefile - Build instructions

## Task 1 - How to Run
cd task1
make
sudo ./backend &
./frontend

## Task 2 - How to Run
cd task2
make
./sandbox ./test_binary

## Environment
- OS: Kali Linux
- Compiler: GCC
- Language: C (C11 standard)

## Author
Name: Sugam
Module: ST5039CMD Programming and Operating Systems
