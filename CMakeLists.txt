cmake_minimum_required(VERSION 3.10)
	project(dnwiki)
	option(MYCOMPDIR "Compile dnwiki" ON)
	add_executable(sshfind main.c)

	target_link_libraries(sshfind PUBLIC "-latomic -ljansson -lssh2 -lavutil")
