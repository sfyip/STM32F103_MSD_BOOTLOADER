/*************************************************************************************
# Released under MIT License
Copyright (c) 2020 SF Yip (yipxxx@gmail.com)
Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************/

#ifndef _IHEX_PARSER_H_
#define _IHEX_PARSER_H_

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_IHEX_DEBUG_OUTPUT        0u          // Output parse status

typedef bool(*ihex_callback_fp)(uint32_t addr, const uint8_t *buf, uint8_t bufsize);

void ihex_reset_state(void);                        // reset state machines, callback function is kept
bool ihex_parser(const uint8_t *steambuf, uint32_t size);
void ihex_set_callback_func(ihex_callback_fp fp);   // Callback function will be triggered at the end of recordtype 'Data'

#endif
