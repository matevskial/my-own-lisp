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
* [] - consider replacing editline with linenoise
* [v] - Experiment with different ways of reading stdin when not using editline
  * try to use fgets instead of scanf(see notes on scanf) https://thelinuxcode.com/read-lines-stdin-c-programming/
    * readline(from editline) also does not impose limit of reading, so same issue as scanf
* [] - Add target for windows and build and run on windows
