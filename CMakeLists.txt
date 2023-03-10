cmake_minimum_required(VERSION 3.5)

project(hello-world)

# Add the source file to the project
add_executable(hello-world main.c)

# Install the executable to /usr/bin
install(TARGETS hello-world DESTINATION /usr/bin)

