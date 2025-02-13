#pragma once

#include <stdio.h>

/**
 * Note: The content being read in the buffer will not include the \n
 * @param prompt
 * @param buff
 * @param size
 * @return
 */
char *read_line_stdin(const char *prompt, char *buff, size_t size);
