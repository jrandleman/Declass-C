# Declass-C
## Declassifier Enables Classes in C by Preprocessing & Compiling Files!
#### **_Ctors/Dtors, Member Default Values/Allocation, Methods, Object Arrays/Ptrs/Containment, Smart Ptrs, Cola Polymorphism, Cola Default Argument Values, and more!_**
-------------------------------------------------------------------------

## Using the Declassifier:
### Compile [declass.c](https://github.com/jrandleman/Declass-C/blob/master/declass.c) (_add "`-l`" for [class info](#-l-output-for-the-simple-sample-stack-class)!_): 
```c
$ gcc -std=c99 -o declass declass.c
$ ./declass yourFile.c // ./declass -l yourFile.c
```
### Command-Line Arguments:
**1.** `-l`: _Show Parsed Class-Object & Cola-Overloading/Default-Value Data!_</br>
**2.** `-save-temps`: _Save the Temp File Made Prior to Passing File to_ `cola.c` _!_</br>
**3.** `-no-compile`: _No Autonomous Compilation (like_ "[`#define DECLASS_NCOMPILE`](#declass-cs-preprocessor-specialization-macro-flags)"_) for converted files!_</br>
**4.** `-mortal-errors`: _Ask client to quit or continue at fatal parsing errors (rather than self-terminating)!_</br> 
* _Invocation:_ `./declass <cmd line flag> yourFile.c`</br>
* _Can Combine Any of the Above, so Long as_ `yourFile.c` _is the Last Arg:_
  * ***IE VALID:*** `$ ./declass -no-compile -l -save-temps yourFile.c`
  * ***INVALID:*** `$ ./declass -no-compile -l yourFile.c -save-temps`
### Implementation:
**1.** _Processed C programs using classes are copied with a_ "`_DECLASS`" _extension & converted to valid C_</br>
**2.** _Unless deactivated, see_ [Macro Flag (9)](#declass-cs-preprocessor-specialization-macro-flags)_, programs are then passed to my_ [`cola.c`](#enables-my-colac-polymorphic-parser-by-default) _polymorphic parser_</br>
**3.** _Converted programs are automatically compiled in_ `C11`_'s standard via_ `GCC` _as a binary file w/ the original name_</br>
  * `yourFile.c`_=declass=>_`yourFile_DECLASS_PRECOLA.c`_=cola=>_`yourFile_DECLASS.c`_=compile=>_`yourFile`
  * [Macro Flags 7-8](#declass-cs-preprocessor-specialization-macro-flags) _can disable both the_ `C11` _standard as well as automatic compiling as a whole_
### References:
* _Provided_ [declass_SampleExec.c](https://github.com/jrandleman/Declass-C/blob/master/declass_SampleExec.c) _demos classes, and_ [declass_SampleExec_DECLASS.c](https://github.com/jrandleman/Declass-C/blob/master/declass_SampleExec_DECLASS.c) _shows conversion_
* _Adhere to the_ [10 Caveats](#declass-cs-10-caveats-straight-from-declassc) _& use_ [declass_SampleExec.c](https://github.com/jrandleman/Declass-C/blob/master/declass_SampleExec.c) _as an operations reference!_
--------------
## Declass-C's 10 Caveats, Straight From "`declass.c`":
* _**Note**: whereas 0-2 pertain to formatting, 3-9 relate to restricted class operations with possible alternatives_
```c
/*****************************************************************************
 *                 -:- DECLASS.C 10 CAVEATS WRT CLASSES -:-                 *
 *   (0) RESERVED: "DC_" PREFIX,"this" PTR,"CONST","INLINE", & STORAGE VARS *
 *   (1) DECLARE CLASSES GLOBALLY & OBJECTS LOCALLY (NEVER IN STRUCT/UNION) *
 *   (2) DECLARE MEMBERS/METHODS USED IN A METHOD ABOVE ITS DECLARATION     *
 *   (3) DECLARE CLASS MEMBERS, METHODS, & OBJECTS INDIVIDUALLY:            *
 *       (*) IE NOT:   "className c(), e();"                                *
 *       (*) RATHER:   "className c(); <press enter> className e();"        *
 *   (4) NO NESTED CLASS DECLARATIONS NOR METHOD INVOCATIONS:               *
 *       (*) IE NOT:   "someObj.method1(someObj.method2());"                *
 *       (*) RATHER:   "int x = someObj.method2(); someObj.method1(x);"     *
 *   (5) CLASS ARRAYS RECEIVED AS ARGS MUST BE DENOTED W/ "[]" NOT "*":     *
 *       (*) IE NOT:   "func(className *classArr){...}"                     *
 *       (*) RATHER:   "func(className classArr[]){...}"                    *
 *   (6) NO PTR TO OBJECT ARRAY OR VISE-VERSA (UNDEFINED BEHAVIOR):         *
 *       (*) IE NOT:   "className (*ptrToArrObj)[10];"                      *
 *       (*) RATHER:   ptr to an obj w/ array of obj member (or vise-versa) *
 *   (7) NO MULTIDIMENSIONAL OBJECT ARRAYS (UNDEFINED BEHAVIOR):            *
 *       (*) IE NOT:   className objMatrix[10][10];                         *
 *       (*) RATHER:   array of objects each w/ array of objects member     *
 *   (8) CONTAINMENT, NOT INHERITANCE: CLASSES CAN ONLY ACCESS MEMBERS &    *
 *       METHODS OF THEIR OWN IMMEDIATE MEMBER CLASS OBJECTS:               *
 *       (*) NOTE:     let "->" denote "can access the members of"          *
 *       (*) IE:       suppose classes c1, c2 & c3: w/ c1 in c2 & c2 in c3. *
 *                     c3 -> c2, and c2 -> c1, BUT NOT c3 -> c1.            *
 *       (*) RATHER:   (1) include a c1 object as a member in c3            *
 *                     (2) mk c2 methods invoking c1 methods: c3 interface  *
 *   (9) ONLY BINARY SINGLE-LINE CONDITIONAL ("?:") OBJECT RETURNS:         *
 *       (*) IE NOT:   "return case ? obj : case2 ? obj2 : obj3;"           *
 *       (*) RATHER:   "if(case) return obj; return case2 ? obj2 : obj3;"   *
 *****************************************************************************/
```
--------------
## Enables My "[`smrtptr.h`](https://github.com/jrandleman/C-Libraries/tree/master/Smart-Pointer)" Library By Default:
* _Improves upon_ "`stdlib.h`"_'s_ "`malloc`" _,_ "`calloc`" _,_ "`realloc`" _, and_ "`free`" _by automating garbage collection_ 
* _Improves upon_ "`assert.h`"_'s_ "`assert`" _function to free smart pointers prior exiting_
  * "`smrtptr.h`"_'s functions work exactly like_ `stdlib.h`/`assert.h`_'s with each prefixed by_ "`smrt`"
  * _Learn more about_ "`smrtptr.h`" _by checking it out in my_ [C-Library](https://github.com/jrandleman/C-Libraries)  _repository_
* _Disable_ "`smrtptr.h`"_'s default inclusion via_ [Macro Flag (2)](#declass-cs-pre-preprocessor-specialization-macro-flags)
--------------
## Enables My "[`cola.c`](https://github.com/jrandleman/Cola)" Polymorphic Parser By Default:
* `cola.c` _(C Overloaded Length Arguments) parser is automatically applied to converted files prior to compiling_
* _Allows for fcn & macro polymorphism (where 2+ share the same name) so long as they take different #'s of args!_
  * _thus "ctor"s_ **CAN** _be overloaded but "dtor"s_ **CANNOT**
* _Also allows default argument values for functions and methods (**not macros!**)!_
  * _Args w/ default values must be listed **last** in a fcn/method's arg list!_</br>
  * _Use the **["ODV GUIDELINE"](#cola-cs-odv-guideline-to-combining-overloads-w-default-arg-values)** below to help avoid overloading ambiguities when combined with default values!_
* _Disable_ "`cola.c`"_'s default application via_ [Macro Flag (9)](#declass-cs-pre-preprocessor-specialization-macro-flags)
#### _Cola-C's 8 Caveats (wrt `declass.c`'s integration):_
```c
/*****************************************************************************
 *                         -:- COLA.C 8 CAVEATS -:-                         *
 *   (*) NOTE: a "COLA INSTANCE" = a fcn/macro overload OR fcn w/ dflt vals *
 *   (0) NO VARIADIC COLA INSTANCES                                         *
 *   (1) NO FCN PTRS POINTING TO COLA INSTANCES                             *
 *       (*) can't determine overloaded arg # from only overloaded fcn name *
 *   (2) NO REDEFINING COLA INSTANCE NAME TO OTHER VARS REGARDLESS OF SCOPE *
 *   (3) NO OVERLOADED MACROS CAN EVER BE "#undef"'d                        *
 *   (4) ONLY COLA INSTANCES DEFINED/PROTOTYPED GLOBALLY WILL BE RECOGNIZED *
 *   (5) ONLY FUNCTIONS MAY BE ASSIGNED DEFAULT VALUES - NEVER MACROS!      *
 *   (6) NO ARG W/ A DEFAULT VALUE MAY PRECEDE AN ARG W/O A DEFAULT VALUE   *
 *       (*) args w/ default values must always by last in a fcn's arg list *
 *   (7) FCN PROTOTYPES TAKE PRECEDENT OVER DEFINITIONS WRT DEFAULT VALS    *
 *       (*) if a fcn proto has default vals but its defn doesn't (or vise  *
 *           versa) fcn will be treated as if both had the default vals     *
 *       (*) if a fcn proto has DIFFERENT default vals from its defn, the   *
 *           fcn's proto default vals are treated as the only default vals  *
 *****************************************************************************/
```
#### _Cola-C's "ODV GUIDELINE" to Combining (O)verloads w/ (D)efault Arg (V)alues:_
* _Defines how to mix arg-number based overloading w/ default values w/o ambiguity_</br>
```c
/*****************************************************************************
 *  -:- ODV GUIDELINE TO COMBINE (O)VERLOADS W/ (D)EFAULT ARG (V)ALUES -:-  *
 * >> overload definitions must satisfy 1 of the following:                 *
 *    (1) an overload's # of non-dflt args must exceed the # of cumulative  *
 *        args (both dflt & not) of all other overloaded instances          *
 *    (2) an overload's # of cumulative args (both dflt & not) must be less *
 *        than the # of non-dflt args of all other overloaded instances     *
 *****************************************************************************/
```
--------------
## Using Constructors (Ctors) & Destructors (Dtors):
### Formatting:                                                           
* **Ctors are denoted as a _typeless method with their class' name_:**     
  * _Can initialize contained object members as a default value_
  * _User-invoked in object declaration, declass.c will automatically* apply default values first_
    * _***Note**: declass.c **only** applies default values to object ptrs if declared as allocated & constructed_
  * _Can take arguments_
* **Dtors are denoted like Ctors, _but prefixed with '~'_:**       
  * _Container objects automatically dtor any member objects first when destroyed_
  * _Either invoked explicitly by user or autonomously by_ `declass.c` _once object out of scope_
  * _**Never takes arguments!**_
* **For Both Ctor's & Dtor's:**
  * _**Never** have a "return" value (being typeless)!_
  * _Can be explicitly invoked by user (suppose object_ "`objName`" _& class_ "`className`")_:_
    * _**Dtor**: destroy object_ "`objName`" _immediately:_ `~objName();`
    * _**Ctor**: return ctor'd_ "`className`" _object instance:_ `className(args);`
      * _so-called "dummy" ctors don't handle specific objects, but return a single "dummy" ctor'd object_
      * _since they only return a single object, can only init an object array's elts (**not** the entire array)_
      * _**except for ptrs**,_ "`className objName(args);`" _==_ "`className objName = className(args);`"
      
### Default Properties:
* **Default object Ctors/Dtors provided if left undefined by user**
* **3 kinds of objects are never dtor'd in their immediate scope:**       
  1) _**Object Arguments**: passed object gets dtor'd instead at the end of their declaration's scope_
  2) _**Returned Objects**: assumed being assigned as a value that's dtor'd externally_
  3) _**"immortal" Objects:** never dtor'd,_ [see below](#the-immortal-keyword) _to learn more_
     * _**Note**: (ii) and (iii) can be dtor'd via_ [Macro Flags 3-4](#declass-cs-pre-preprocessor-specialization-macro-flags)

### Object Declarations:
* _**Note**: suppose class_ "`className`"_, object_ "`objName`"_, & an object memory allocation function_ "`alloc`"
* **Using Only Default Values:**
  * _**Single Object**:_ `className objName;`
  * _**Object Array**:_ `className objName[size];`
* **Using Ctors (declass.c automatically assigns object default values first):**
  * _**Single Object**:_ `className objName(args);`
  * _**Object Array**:_ `className objName[size](args);`
* **Object Pointers & Initializing them with Ctors/Dflts/Neither:**       
  * _**"Dangling" Ptr**:_ `className *objName; // neither Ctor nor Dflt (default) values applied`
  * _**Ctor'd Ptr**:_ `className *objName(args); // only advised if Ctor also allocates memory`
  * _**Ctor'd & Alloc'd Ptr**:_ `className *objName(args) = alloc(sizeof(className)); // best choice`
* **Object Pointer Best Practices to Reduce Risk of Errors:**       
  * _**"Dangling" Ptr**: keep_ [immortal](#the-immortal-keyword) _unless class specifically designed for ptrs_
  * _**Non-Dangling**: allocate memory & Ctor upon declaration_
--------------
## The "immortal" Keyword:
### Objects Declared as "immortal" are Never Dtor'd:
* _**Declaration**:_ `immortal className objName(args);`
* _**Contained objects** can **also** be immortal!_
* **WARNING: *an immortal object's contained members are also immortal!***
### Object Arguments are Always "immortal":
* _As placeholders, the passed object they represent aren't out of scope once the fcn ends (no double Dtor)_
* [Macro Flag (3)](#declass-cs-preprocessor-specialization-macro-flags) _below disables **all** immortal objects **except** object arguments_
--------------
## Declass-C's Preprocessor Specialization Macro Flags:
### By Precedence:
0) ```c 
   #define DECLASS_IGNORE       // "declass.c" won't convert file
   ```
1) ```c
   #define DECLASS_STRICTMODE   // enables macro flags 2-4
   ```
2) ```c
   #define DECLASS_NSMRTPTR     // disables default inclusion of "smrtptr.h"
   ```
3) ```c
   #define DECLASS_NIMMORTAL    // disables "immortal" keyword
   ```
4) ```c
   #define DECLASS_DTORRETURN   // also Dtor returned objects
   ```
5) ```c
   #define DECLASS_NOISYSMRTPTR // printf an alert whenever smrtptr.h allocates or frees
   ```
6) ```c
   #define DECLASS_NDEBUG       // disable smrtptr.h "smrtassert()" statements
   ```
7) ```c
   #define DECLASS_NCOMPILE     // declass.c declassifies but DOESN'T GCC compile converted file
   ```
8) ```c
   #define DECLASS_NC11         // GCC compiles declassified file w/o "-std=c11"
   ```
9) ```c
   #define DECLASS_NCOLA        // disables "cola.c" overload/default-value parser invocation
   ```
### Defining Custom Memory Allocation Functions:
* `declass.c` _relies on identifying memory allocation functions to not apply dflt vals to garbage memory_
* `declass.c` _de facto recognizes_ `malloc`_,_ `calloc`_,_ `smrtmalloc`_, and_ `smrtcalloc`
* _Users can declare custom object memory allocation fcns to be recognized by_ `declass.c` _via the macro:_
```c
#define DECLASS_ALLOC_FCNS // list custom alloc fcns here, as TOKENS not as STRINGS
```
* _**Note:**_ **`declass.c`** _**assumes all allocation functions return**_ **`NULL`** _**or terminate the program upon failure!**_
--------------
## Creating Header Files To Be Parsed By `declass.c`:
### Overview:
* `declass.c` _supports header files with classes &_ [`cola.c`](#enables-my-colac-polymorphic-parser-by-default) _overloading/default values!_
* _Must be prefixed with_ `"DECLASS_H_"` _or_ `"declass_h_"`_, & have the_ `".h"` _file extension_
* _Allows writing 1 class for several files, reducing redundant code_
### Implementation:
* _Header files are prepended to the main file for a monolithic code base_
  * _Result must **cumulatively** be less bytes than the_ `MAX_FILESIZE` _(default 1 megabyte) macro!_
* _Headers **not** prefixed_ `"DECLASS_H_"` _or_ `"declass_h_"` _will **not** be prepended to main codebase by_ `declass.c`
### Example:
```c
#include "DECLASS_H_yourHeader.h" // valid header
```
--------------
## A Simple Sample Stack Class:
* _**Note**:_ [declass_SampleExec.c](https://github.com/jrandleman/Declass-C/blob/master/declass_SampleExec.c) _, has **much** more on object ctors/dtors, containment, arrays, ptrs, default memory allocation, struct/fcn-ptr members, smrtptr.h autonomous freeing, methods using "this" ptr, and more!_
* _**Note**: for those unfamiliar with OOP, "members" are class variables and "methods" are class functions_
```c
#include <stdio.h>
#include <stdbool.h>

class Stack {
  int *arr = smrtmalloc(sizeof(int) * 10); // "smrtmalloc" via smrtptr.h handles freeing
  int len;      // empty member values default 0
  int max = 10; // all "Stack" objects will now default to "max" = 10

  // note that class methods can refer to their class' members & other methods
  // without any prefixes, as declass.c will automatically detect which class
  // object is invoking the method & implement the appropriate operations.

  void push(int elt) { // a class "method"
    if(len == max) {   // "len" and "max" invoke the current class' members
      max *= max;
      arr = smrtrealloc(arr, sizeof(int) * max); // via smrtptr.h
    } else
      arr[len++] = elt;
  }
  bool pop(int *elt) {
    if(len == 0) return false;
    *elt = arr[--len];
    return true;
  }
  bool top(int *elt) {
    if(len == 0) return false;
    *elt = arr[len-1];
    return true;
  }
  void show() { for(int i = 0; i < len; ++i) printf("%d ", arr[i]); printf("\n"); }
  int size() { return len; } // return a local member value from method

  // create class constructors by making a "typeless" method w/ the same name of the class
  // => gets invoked at every object delcaration so long as "(<args>)" are provided,
  //    otherwise object only get default values without calling its constructor.
  Stack(int array[], int length) {
    for(int i = 0; i < length; ++i) 
      push(array[i]);
  }

  // class destructor, formatted like a ctor but prefixed w/ '~',
  // is invoked once an object is out of scope
  ~Stack() {
    printf("Stack object destroyed!\n");
  }
}


// note that class objects invoke members/methods by '.' or '->'
// notation as per whether they aren't/are a class pointer


int main() {
  // Single "Stack" object initialized with default values
  printf("Working with a single \"Stack\" object initializaed with its default values:\n");
  Stack myStack;   // declare object
  myStack.push(8); // invoke "Stack" object's method
  myStack.push(10);
  myStack.push(12);
  printf("Pushed 8, 10, then 12:\n");
  myStack.show();
  int x;
  bool popped = myStack.pop(&x);
  if(popped) printf("Popped Value: %d\n", x);

  myStack.show();
  myStack.push(100);
  printf("Pushing 100:\n");
  myStack.show();
  printf("Pushing 888:\n");
  myStack.push(888);
  myStack.show();

  int y;
  bool topped = myStack.top(&y);
  if(topped) printf("Top Value: %d\n", y);
  int size = myStack.size(); // invoke Stack object's member in the "printf" below:
  printf("Stack's size: %d, Stack's current max capacity: %d\n", size, myStack.max);


  // Single "Stack" object initialized with default values & constructor
  printf("\nInitializing a \"Stack\" object via its default values & class constructor:\n");
  int arr[20] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181};
  Stack newStack(arr, 20);
  printf("\"Stack\" object made with its class constructor:\n");
  newStack.show();

  return 0;
}
```
--------------
## '`-l`' Output for the Simple Sample Stack Class:
* _**Note**: helps confirm whether or not your class code converted as anticipated!_ 
  * _refer to the_ [10 Caveats](#declass-cs-10-caveats-straight-from-declassc) _&_ [declass_SampleExec.c](https://github.com/jrandleman/Declass-C/blob/master/declass_SampleExec.c) _otherwise!_
```
--=[ TOTAL CLASSES: 1 ]=--=[ TOTAL OBJECTS: 2 ]=--

CLASS No1, Stack:
 L_ MEMBERS: 4
 |  L_ *object_has_been_destroyed_flag
 |  L_ *arr (( ALLOCATED MEMORY ))
 |  L_ len
 |  L_ max
 L_ METHODS: 7
 | L_ push()
 | L_ pop()
 | L_ top()
 | L_ show()
 | L_ size()
 | L_ Stack() (( CONSTRUCTOR ))
 | L_ ~Stack() (( DESTRUCTOR ))
 L_ OBJECTS: 2
   L_ myStack
   L_ newStack
```
