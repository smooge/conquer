# Possible plan for Phase 4 error reduction

Phase 4 is going to take a long time and will require multiple sub
phases with many checkpoints per file being modernized. This plan is not
final and may have major flaws. Please review and come up with
alternatives or additional session plans.

At some point in this cleanup, we should also incorporate clang-tidy and clang

```
clang-tidy old_code.c --checks='-*,bugprone-*,readability-simplify-boolean-expr,modernize-use-trailing-return-type'
```

Clang-tidy checks should be iteratively added so that we do not over fill our context windows.

## Standard subphase work

1. Get a list of all files which need to be compiled and what fixes were found per file.
2. Sort the number of problems per file and start with the highest problem count down.
3. Use a checkpoint methodology for fixing problems in a file.
   1. Fix compile errors first.
   2. Break down the warning types found in a file and only fix one type of warning at a time.
   3. Break down the number of functions which need to be 'fixed' per session. Do no more than 3 functions per session.
   4. Always test compile after doing a set of fixes
   5. Always `git commit` the file after the compile has fixed a warning
4. When a subphase is done compile a report on the problems initially found, the fixes needed to be done, and the expected results from said fixes.

**IMPORTANT**: If problems start repeating to not be fixed by solutions always ask for help
**IMPORTANT**: Always think hard on each problem. Speed is not needed. Correctness and thoroughness is.

## When to use different compilers

It has been noted by other developers that using gcc and clang can help
give more information on what to fix and why. In some cases, gcc will
give a general warning while clang will give a better description, and
in other cases it will be the other way. The general idea should be to
use gcc first and when needed use clang to improve the idea on the
problem or what it is.

## Subphase 0: Inventory and base line test
- Inventory all .c and .h files
- Test basic compilation capability
- Generate comprehensive error/warning report
- Identify blocking vs. non-blocking issues
- Create file prioritization matrix
- Create standard files which could speed this up
  - **`analyze_compilation.py`**: Parse GCC output and categorize issues
  - **`prioritize_files.py`**: Sort files by complexity and error count
  - **`track_progress.py`**: Update status files automatically


## Subphase 1: Simplest compilation test and cleanup

For the first round of cleanup of warnings and errors, just use the
simplest gcc without any extra warnings enabled. In fact we will want to disable some flags which are always on.

Something like
```
for file in *.h *.c; do
	/usr/bin/gcc -O2 -g -Wno-traditional-conversion -Wno-old-style-declaration -std=c99 -c ${file} -o /tmp/foo.o >> /tmp/gcc_problems_01 2>&1
done
```

Start with the header files and then move through other programs to see
what common errors and warnings there are. Cleaning up these at this
stage will result in a lot less later noise to wade through.

## Subphase 2: Add back in conversion and old style declarations

```
for file in *.h *.c; do
	/usr/bin/gcc -O2 -g -std=c99 -c ${file} -o /tmp/foo.o >> /tmp/gcc_problems_01 2>&1
done
```


## Subphase 3: Add in minimal set or warnings.

```
for file in *.h *.c; do
	/usr/bin/gcc -O2 -g -std=c99 -Wall -c ${file} -o /tmp/foo.o >> /tmp/gcc_problems_02 2>&1
done
```

Also incorporate `clang -Wall -std=c99` to get better ideas on what could be done here.



## Subphase 4: Add in standard set of warnings

```
for file in *.h *.c; do
	/usr/bin/gcc -O2 -g -std=c99 -Wall -Wextra -Wpedantic -Wformat -Wformat=2 -c ${file} -o /tmp/foo.o >> /tmp/gcc_problems_03 2>&1
done
```

Also incorporate `clang -Wall -Wextra -std=c99` to get better ideas on what could be done here.


## Subphase 5: Move from c99 to later C standards

At this point we want to repeat the last set of warnings and tests but
just move the C standard to a newer version. This could be C11, then C17
and then C2x to get an idea of the growth of problems or it can be the
big jump

```
for file in *.h *.c; do
	/usr/bin/gcc -O2 -g -Wall -Wextra -Wpedantic -Wformat -Wformat=2 -std=c2x -D_POSIX_C_SOURCE=200809L -c admin.c -o /tmp/foo.o >> /tmp/gcc_problems_04 2>&1
done
```

**QUESTION**: What clang additions can we add here?

**ANSWER**: Clang would move from `-std=c99` to `-std=c2x -D_POSIX_C_SOURCE=200809L`

## Subphase 6: Incorporate clang-tidy to detect other items as needed

Using `clang-tidy` will help speed up finding and fixing issues. It
incorporates conversion of many K&R'isms to newer C standards and can
point out problems that may be helpful for later stages.

Step 1: Analyze the Code

First, you run clang-tidy on your file. You need to specify a list of
checks you want to run. A good starting point for modernization is the
modernize-, bugprone-, and readability- checks. The * acts as a wildcard
to include all checks in a category.


```
clang-tidy old_code.c --checks='-*,bugprone-*,readability-simplify-boolean-expr,modernize-use-trailing-return-type'
```

--checks='-*' disables all checks by default.

bugprone-* enables checks that find likely bugs.

readability-* and modernize-* enable checks for style and modernization.

Output:

clang-tidy will generate output highlighting the problems.

Step 2: Automatically Fix the Issues
Many clang-tidy checks, especially those related to formatting and simple refactoring, can automatically apply fixes. You can tell clang-tidy to apply these fixes using the --fix option.

Run the command again with --fix:

```
clang-tidy old_code.c --fix --checks='-*,bugprone-*,readability-simplify-boolean-expr,modernize-use-trailing-return-type'
```

However not all will be fixed because there may be more than one fix to try.

Step 3: Test that the fixes still compile without more errors or problems.

Run a compile to see if the file has more problems or not. If it does, then we need to see if we can fix it or need to do a `git restore` .

## Subphase 7: Final gcc issues.

```
for file in *.h *.c; do
	/usr/bin/gcc -O2 -g -Wall -Wextra -Wpedantic -Wformat -Wformat=2 -std=c2x -D_POSIX_C_SOURCE=200809L -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fsanitize=address -fsanitize=undefined -fanalyzer -Wstrict-prototypes -Wold-style-declaration -c admin.c -o /tmp/foo.o >> /tmp/gcc_problems_05 2>&1
done
```


At this point we will need to start really addressing various data type
warnings and such. We will also need to see if we have found enough
issues to consider this phase done and put these last two steps to
another phase.

Data Types:

Use `size_t` for all object sizes and array indices. It's an unsigned
integer type guaranteed to be large enough to hold the size of any
object.

Use `int32_t`, `uint32_t`, `int64_t`, `uint64_t` from <stdint.h> when
you need a specific integer size.

Be cautious with long. Its size can vary (32-bit on some systems, 64-bit on others).

Work file by file on this


## Subphase 8: Add in more items

The final section is working through the warnings that `-Wtraditional`
and `-Wtraditional-conversion` give. This is complicated because it will
both warn about where a post-ANSI 1980 item won't work in K&R and also
where some remaining K&R item might cause problems in

```
for file in *.h *.c; do
/usr/bin/gcc -O2 -g -Wall -Wextra -Wpedantic -Wformat -Wformat=2 -Wconversion -Wsign-conversion -std=c2x -D_POSIX_C_SOURCE=200809L -Wimplicit-fallthrough -fsanitize=address -fsanitize=undefined -fanalyzer -Wstrict-prototypes -Wold-style-declaration -Wtraditional -Wtraditional-conversion -c ${file} -o /tmp/foo.o >> /tmp/gcc_problems_06 2>&1
```

## Subphase ??: Use clang-tidy to point out issues

This is an area needing to be reviewed.
