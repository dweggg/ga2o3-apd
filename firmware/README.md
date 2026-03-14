# Firmware
All the embedded code will be tracked here

## Naming
The most important consistency rules are those that govern naming. The style of a name immediately informs us what sort of thing the named entity is: a type, a variable, a function, a constant, a macro, etc., without requiring us to search for the declaration of that entity. The pattern-matching engine in our brains relies a great deal on these naming rules.

Style rules about naming are pretty arbitrary, but we feel that consistency is more important than individual preferences in this area, so regardless of whether you find them sensible or not, the rules are the rules.

For the purposes of the naming rules below, a "word" is anything that you would write in English without internal spaces. Either words are all lowercase, with underscores between words ("snake_case"), or words are mixed case with the first letter of each word capitalized ("camelCase" or "PascalCase").

### File names
Filenames should be all lowercase and can include underscores (_). Follow the convention that your project uses. If there is no consistent local pattern to follow, prefer "\_".

Examples of acceptable file names:
 - my_useful_class.c

In general, make your filenames very specific. For example, use http_server_logs.h rather than logs.h. A very common case is to have a pair of files called, e.g., foo_bar.h and foo_bar.cc, defining a class called FooBar.

### Type Names
The names of all types — classes, structs, type aliases, enums, and type template parameters — have the same naming convention. Type names should start with a capital letter and have a capital letter for each new word. No underscores. For example:
```c
typedef struct {...} UrlTablePropertiesTypeDef

typedef enum {...}  UrlTablePropertiesTypeDef
```

### Variable Names
The names of variables (including function parameters) and data members are snake_case (all lowercase, with underscores between words). Data members of classes (but not structs) additionally have trailing underscores. For instance: a_local_variable, a_struct_data_member, a_class_data_member_.
For example:
```c
char *table_name;  // OK - snake_case.
char *tableName;   // Bad - mixed case.
```

### Struct Data Members
Data members of structs, both static and non-static, are named like ordinary nonmember variables. They do not have the trailing underscores that data members in classes have.

```c
typedef struct  {
  char *name;
  int num_entries;
}UrlTablePropertiesTypeDef;
```


### Function Names
Ordinarily, functions follow PascalCase: start with a capital letter and have a capital letter for each new word. Usually function should start with a verb.
```c
AddTableEntry()
DeleteUrl()
OpenFileOrDie()
```

### Enumerator Names
Enumerators should be declared as a typedef, same as structure.
Enumerators (for both scoped and unscoped enums) should be named like constants, not like macros. That is, use kEnumName not ENUM_NAME.

```c
typedef enum  {
  OK = 0,
  OUT_OF_MEMORY,
  MAL_FORMED_INPUT,
}UrlTableErrorTypeDef;
```

### Macro Names
They should be named with all capitals and underscores, and with a project-specific prefix.
```c
#define MYPROJECT_ROUND(x) ...
```

## Comment Style
Use either the // or /* */ syntax, as long as you are consistent.

While either syntax is acceptable, // is much more common. Be consistent with how you comment and what style you use where.

### Comments for Macros
Every macro needs a comment to briefly introduce what does it means or why do we need it, for example:
```c
#define SYSCTL_PLLCLK (200000000)	//The chip main clock is 200MHz
```

### Comments for Variables
If it is a global variable inside source file(Usually we do NOT use global variables, but just in case), comment is required to explain what does it means or why do we need it when the unit of variable naming is not clear or have a actual unit, like temperature or phase to phase current instantaneous. For example:
```c
uint32_t *uart_rxbuffer[xxx]; // rx buffer for SCI communication
float32_t mosfet_leg_a_top_temp // temperature of leg A top MOSFET in degree Celsius
```

### Comments for Functions
All functions need a Doxygen-style comment before it, which briefly introduce the function of it, what caller should be aware of, the return type and explain the return param(if not void). Please also naming the params clearly, explaining the params is better. However if the input params is a pointer and the function is expected to modify it, explaining the params is a MUST and label with [in]/[out]/[in,out]. For example:
```c
//@brief Init the task scheduler, use after hardware is initialized
//@params [in] TIMER_VARS timer: Timer that is used as the clock source of scheduler
//@return @sa HAL_StatusTypeDef, return HAL_OK if successfully initialized
HAL_StatusTypeDef InitTaskScheduler(TIMER_VARS timer)

//@brief performance a pi calculation
//@params [in,out] PIDTypeDef *pi
//@return none
void CalculatePI(PIDTypeDef *pi)
``` 

## Formatting
### Spaces vs. Tabs

Use only spaces, and indent 4 spaces at a time.

We use spaces for indentation. Do not use tabs in your code. You should set your editor to emit spaces when you hit the tab key.

## Include guard
It's a preprocessor macro, All of it is preprocessor syntax, that basically says, if this macro has not already been defined, define it and include all code between the #ifndef and #endif. What it accomplishes is preventing the inclusion of file more than once. The include guard should be named with all capitals of the file name and underscores with double underscores wrapped. For example:

```c

#ifndef __YOUR_FILE_NAME_H__
#define __YOUR_FILE_NAME_H__
///your code here
#endif
```