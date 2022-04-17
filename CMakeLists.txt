cmake_minimum_required(VERSION 3.21)
project(jsonTS)

set(CMAKE_CXX_STANDARD 17)

add_executable(jsonTS main.cpp json.cpp json.h json_reader.cpp json_reader.h map_renderer.cpp map_renderer.h request_handler.cpp request_handler.h svg.cpp svg.h transport_catalogue.cpp transport_catalogue.h geo.h geo.cpp domain.cpp domain.h)
