add_executable(test01 test01.cpp)
target_link_options(test01 PRIVATE -fsanitize=thread)
target_compile_options(test01 PRIVATE -Wall -g -fPIC -pie -fsanitize=thread)
add_test(
	NAME test01
	COMMAND test01
)
