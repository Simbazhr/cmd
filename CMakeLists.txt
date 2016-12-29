cmake_minimum_required(VERSION 3.4)
project(cmdline)
add_executable(main client.c msg.h msg.c)
add_executable(server server.c msg.h msg.c)
