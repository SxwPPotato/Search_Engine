cmake_minimum_required(VERSION 3.20)

add_executable(SpiderApp
	main.cpp
	http_utils.h
	http_utils.cpp
	
	parser_html.cpp
	parser_html.h 
	Database.cpp
	Database.h
    parser_ini.h 
    parser_ini.cpp   
	Exeption.h
	Task.cpp 
	Task.h
	http_client_async.cpp
	http_client_async_ssl.cpp)

target_compile_features(SpiderApp PRIVATE cxx_std_17) 

target_link_libraries(SpiderApp pqxx)

target_link_libraries(SpiderApp OpenSSL::SSL)

target_include_directories(SpiderApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SpiderApp ${Boost_LIBRARIES})

target_compile_options(SpiderApp PRIVATE /bigobj)