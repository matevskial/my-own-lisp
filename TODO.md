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
