Build:
> make

Usage:
> ./compiler xxx.jack > xxx.xml

For grader convenience: # path sensitive
> sh generate_xml.sh

For fun:
> make compiler_raw
> ./compiler_raw jacks/wrongSquareGame1.jack # wrong statement
> ./compiler_raw jacks/wrongSquareGame2.jack # incomplete file
> ./compiler_raw jacks/wrongSquareGame3.jack # missing semicolon
> ./compiler jacks/wrongSquareGame1.jack
> ./compiler jacks/wrongSquareGame2.jack
> ./compiler jacks/wrongSquareGame3.jack

compiler_raw: The very first version of compiler. No indent, no token debug, no line tracer, no empty container protector, just basic function. One should be sure xxx.jack is correct in syntax or output is unpredictable.

compile_indent: Add blank characters head of each xml recursively for readable propose.

compile_debug_token: Compiler will check whether a token is expected under jack parsing tree before writing. If token is not expected by compiler, program terminates. The parsing tree is hard coded in compile engine by programmer.

compile_debug_line: Besides throwing exception when a unexpected token is met, compiler feedbacks line number and content at which error happens. Additionally, it fixes potentially segmentation fault by terminating program if token list is empty before access to list's front.
