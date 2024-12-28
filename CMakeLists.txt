cmake_minimum_required(VERSION 3.20)

project(CppDiplomProject)

# TODO: Define by your own needs:
set(OPENSSL_ROOT_DIR "C:\\openssl-3.4.0")

set(BOOST_ROOT "С:\\boost_1_86_0")

set(libpqxx_DIR "D:\\1\\libpqxx-7.9.0")

add_compile_definitions(_WIN64_WINNT=0x0A01)

# Boost:
find_package(Boost 1.80.0 REQUIRED COMPONENTS system thread locale)

if(NOT Boost_FOUND)
   message(FATAL_ERROR "Could not find Boost")
endif()


# PQXX
set(PostgreSQL_ADDITIONAL_VERSIONS "16")

add_subdirectory(${libpqxx_DIR} libpqxx-build) 

# OpenSSL
find_package(OpenSSL REQUIRED)

add_subdirectory(spider)

add_subdirectory(http_server)
