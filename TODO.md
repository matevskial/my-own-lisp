# Task tracking

* [] - Use #define to introduce custom definitions for win and unix,mac to not duplicate if in input_reader.c
* [] - Experiment with different ways of declaring editline dependency in meson
  * using find_library 
    * https://groups.google.com/g/mesonbuild/c/FFOD92YNH2Q 
    * https://stackoverflow.com/questions/59769986/meson-how-to-make-find-library-works-with-an-unusual-path
  * using dependency?
* [] - Experiment with different ways of reading stdin when not using editline
  * try to use fgets instead of scanf(see notes on scanf) https://thelinuxcode.com/read-lines-stdin-c-programming/
* [] - Add target for windows and build and run on windows
