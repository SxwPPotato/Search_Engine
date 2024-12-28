cmake_minimum_required(VERSION 3.20)


add_executable(HttpServerApp 
	main.cpp
	Exeption.h 
	parser_ini.cpp
	parser_ini.h
	Database_serv.cpp
	Database_serv.h 
	Server.cpp 
	Interface_serv.h 
	Interface_serv.cpp)

target_compile_features(HttpServerApp PRIVATE cxx_std_17) 

target_link_libraries(HttpServerApp pqxx)  

target_include_directories(HttpServerApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(HttpServerApp ${Boost_LIBRARIES})

target_link_libraries(HttpServerApp OpenSSL::SSL)
