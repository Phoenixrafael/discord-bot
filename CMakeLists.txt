project(labyrinth-bot)
 
add_executable(discord discord.cpp sqlite3.c discord.h sqlite3.h sqlite3ext.h "IO_file.h")

find_package(sleepy-discord CONFIG REQUIRED)

if(WIN32) 
	target_link_libraries(discord sleepy-discord)
else()
	target_link_libraries(discord sleepy-discord dl)
endif()