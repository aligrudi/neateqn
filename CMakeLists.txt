cmake_minimum_required(VERSION 3.0)
project(neateqn C)
add_executable(eqn eqn.c tok.c src.c def.c box.c reg.c sbuf.c)
install(TARGETS eqn)
