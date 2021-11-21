project(labyrinth-bot)
 
add_executable(labyrinth LabyrinthBot.cpp sqlite3.c discord.h sqlite3.h sqlite3ext.h "IO_file.h")
#add_executable(qasino QasinoBot.cpp sqlite3.c discord.h sqlite3.h sqlite3ext.h "IO_file.h")

find_package(sleepy-discord CONFIG REQUIRED)

if(WIN32) 
	target_link_libraries(labyrinth sleepy-discord)
	#target_link_libraries(qasino sleepy-discord)
else()
	target_link_libraries(labyrinth sleepy-discord dl)
	#target_link_libraries(qasino sleepy-discord dl)
endif()
