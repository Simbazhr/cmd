cmake_minimum_required(VERSION 3.4)
project(cmdline)
add_executable(main client.c msg_operation.h msg_operation.c)
add_executable(server server.c msg_operation.h msg_operation.c)
