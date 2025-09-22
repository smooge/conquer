# Data.h Cproto Fixes Needed

**Test Results**: commands.c compilation with new cproto data.h shows significant improvement but 4 issues need resolution.

## Issues Found and Solutions

### 1. Duplicate `place` Function (2 errors)
**Problem**: Two different functions with same name
```
data.h:1023: extern void place(int xloc, int yloc);     // from newlogin.c
data.h:1082: extern void place(char data[]);           // from sort.c
```

**Solution**: Rename one function to avoid conflict. Recommend renaming sort.c version:
```c
/* In sort.c section of data.h, change: */
extern void place(char data[]);
/* To: */
extern void place_data(char data[]);

/* Then in sort.c, rename the function definition accordingly */
```

### 2. Missing L_PTR Typedef (2 errors)
**Problem**: L_PTR type used in sort.c but not defined in data.h
```
data.h:1084: extern L_PTR build_node(char data[], L_PTR nptr);
```

**Solution**: Add typedef before sort.c section in data.h:
```c
/* Before sort.c section, add: */
typedef struct {
    char data[80];
    struct L_DATA *next;
} L_DATA, *L_PTR;
```

### 3. Missing Function Declarations (4 errors)
**Functions not declared in data.h but needed:**

```c
/* Add to misc.c section: */
extern void reset_god(void);           // defined in misc.c:2581
extern int get_country(void);          // defined in misc.c:2372

/* Add to magic.c section: */
extern int unitvalid(int type);        // defined in magic.c:1114

/* Add to misc.c section: */
extern int move_file(char *from, char *to);  // defined in misc.c:84
```

## Quick Fix Commands

```bash
# Add missing functions to data.h misc.c section:
# After line with other misc.c functions, add:
extern void reset_god(void);
extern int get_country(void);
extern int move_file(char *from, char *to);

# Add to magic.c section:
extern int unitvalid(int type);

# Add L_PTR typedef before sort.c section:
typedef struct L_DATA {
    char data[80];
    struct L_DATA *next;
} L_DATA, *L_PTR;

# Fix duplicate place function in sort.c section:
# Change: extern void place(char data[]);
# To:     extern void place_data(char data[]);
```

## Expected Results After Fixes
- commands.c should compile with only harmless VERSION redefinition warning
- All implicit function declaration errors resolved
- No more conflicting function name errors

## Verification Command
```bash
gcc -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -DDEFAULTDIR="/home/ssmoogen/conquer/lib" -DEXEDIR="/home/ssmoogen/conquer/bin" -DPATCHLEVEL="12" -DLOGIN="ssmoogen" -DCONQUER -c commands.c -o /tmp/foo.o
```

**Note**: The cproto approach is working well - these are just minor cleanup issues to resolve.
