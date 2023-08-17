# brief: print given messages in a good-looking box
# 
# param: title Title of the box
function(box_message title)
	string(LENGTH ${title} title_length)
	set(padding_count 40)

	set(first_line "┌───── ${title} ")
	set(last_line  "└───────")

	math(EXPR first_line_padding_count "${padding_count} - ${title_length}")
	string(REPEAT "─" ${first_line_padding_count} first_line_padding)
	string(REPEAT "─" ${padding_count} last_line_padding)

	string(APPEND first_line ${first_line_padding})
	string(APPEND last_line ${last_line_padding})

	message(STATUS ${first_line})
	foreach(arg ${ARGN})
		message(STATUS "│ ${arg}")
	endforeach()
	message(STATUS ${last_line})
endfunction()
