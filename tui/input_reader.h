#pragma once

#include <stdio.h>

/**
 * The content being read in the buffer will not include the \n
 * @param prompt
 * @param buff
 * @param size
 * @return number of bytes read in the buffer, 0 if failure reading input
 */
size_t read_line_stdin(const char *prompt, char *buff, size_t size);
