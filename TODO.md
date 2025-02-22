# Task tracking

* [v] - Use #define to introduce custom definitions for win and unix,mac to not duplicate if in input_reader.c
  * use all macros for unix too https://stackoverflow.com/questions/142508/how-do-i-check-os-with-a-preprocessor-directive#8249232
  * https://stackoverflow.com/questions/2989810/which-cross-platform-preprocessor-defines-win32-or-win32-or-win32
  * https://mesonbuild.com/FAQ.html#how-to-add-preprocessor-defines-to-a-target
* [v] - Experiment with different ways of declaring editline dependency in meson
  * using find_library 
    * https://groups.google.com/g/mesonbuild/c/FFOD92YNH2Q 
    * https://stackoverflow.com/questions/59769986/meson-how-to-make-find-library-works-with-an-unusual-path
  * using dependency?
* [] - Consider replacing editline with linenoise
* [v] - Experiment with different ways of reading stdin when not using editline
  * try to use fgets instead of scanf(see notes on scanf) https://thelinuxcode.com/read-lines-stdin-c-programming/
    * readline(from editline) also does not impose limit of reading, so same issue as scanf
* [v] - Add target for windows and build and run on windows
* [v] - Consider refactoring read_line_stdin to not contain preprocessor directives
* [v] - Consider creating custom struct typedefs that represent the ast of `my-own-lisp` in order to 
  * not depend on mpc in interpreter.h and interpreter.c
  * move the function eval in interpreter.c
  * the main method would glue together mpc and eval by translating mpc ast to the ast represented by the custom structs
  * note: this is kind-of done by introducing s-expressions and distinguishing between parsing lisp_value_t and evaluating lisp_value_t
* [] - Use perf profiler with IDE(such as CLion) or valgrind and try to find memory leaks if there are any
  * try introducing memory leak
    * such as not deleting lisp_value_t* before builtin_op returns the evaluated value implemented for chapter 9, see the commit that mentions chapter 9
    * not freing lisp_eval_result itself in lisp_eval_result_delete
* [] - Compare implementations of non-destructive and destructive evaluate_lisp_value(implemented for chapter 9)
  * non destructive evaluates the operation after the next expression is evaluated
  * the destructive implementation evaluates all expressions and then executes the operation
  * see if the way non-desturcitve evaluate is implemented can be used in the destructive evaluate operation
* [] - Consider moving inc(.h) files in the same list as source files
  * check https://github.com/Backseating-Committee-2k/vhdl/blob/main/display/src/meson.build
* [] - Implement q-expressions(chapter 10). I am first reading the chapter, trying to understand the concepts, 
writing some notes, then write code to implement.
  * see if I can use macros to cleanup code implementation 
  * a QEXPR is lisp_value_t* that contains one exactly child of type QEXPR
  * list, head, tail, join, eval take a q-expression, join takes one or more q-expressions
  * head takes the first element(child) of the q expression and deletes the rest
  * tail deletes the first element(child) of q expression and returns the q-expression that contains all the childs except the first
  * example: eval (tail {tail tail {5 6 7}})
    * eval must operate on a q-expression so argument to eval must evaluate to a q-expression
  * after implementing, compare with chapter and implement bonus points
