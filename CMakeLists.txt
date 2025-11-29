cmake_minimum_required(VERSION 3.5)
project(mod-instance-tools)

set(scripts_STAT_SRCS
  src/instance_reset.cpp
  src/autoupdate.cpp
  src/voa_teleport.cpp
)

AC_ADD_SCRIPT("${scripts_STAT_SRCS}")
AC_ADD_CONFIG_FILE(conf/instance_tools.conf.dist)

message(STATUS "-> module 'mod-instance-tools' loaded.")
