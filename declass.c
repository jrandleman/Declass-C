// AUTHOR: JORDAN RANDLEMAN - DECLASSIFIER TO PREPROCESS .C FILES USING CLASSES
// Email jrandleman@scu.edu or see https://github.com/jrandleman for support
/**
 * compile: $ gcc -std=c99 -o declass declass.c
 *          $ ./declass yourFile.c 
 *     (OR) $ ./declass -l yourFile.c // optional '-l' shows class details 
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
// scrape file's contents, put contents into file, & add new filename extension
#define FSCRAPE(BUFF,FNAME)({\
  BUFF[0]='\0';FILE*fptr;if((fptr=fopen(FNAME,"r"))==NULL){exit(EXIT_FAILURE);}\
  while(fgets(&BUFF[strlen(BUFF)],500,fptr)!=NULL);fclose(fptr);\
})
#define FPUT(BUFF,FNAME)\
  ({FILE*fptr;if((fptr=fopen(FNAME,"w"))==NULL){exit(EXIT_FAILURE);}fprintf(fptr,"%s",BUFF);fclose(fptr);})
#define NEW_EXTENSION(FNAME,EXT) ({char*p=&FNAME[strlen(FNAME)];while(*(--p)!='.');strcpy(p,EXT);})
// checks if char is whitespace, or alphanumeric/underscore
#define IS_WHITESPACE(c) (c == ' ' || c == '\t' || c == '\n')
#define VARCHAR(CH) (((CH)>='A' && (CH)<='Z') || ((CH)>='a' && (CH)<='z') || (((CH)>='0' && (CH)<='9')) || (CH) == '_')
// append string to NEW_FILE buffer when splicing in large-scale changes
#define APPEND_STR_TO_NEW_FILE(appending) \
  for(int m_i = 0, len = strlen(appending); m_i < len; ++m_i, *j += 1) NEW_FILE[*j] = appending[m_i];
// initialize array with zero's (wipes garbage memory)
#define FLOOD_ZEROS(arr, len) ({for(int arr_i = 0; arr_i < len; ++arr_i) arr[arr_i] = 0;})
// file, class, & object limitations => all self imposed for memory's sake, increment as needed
#define MAX_FILESIZE 1000001 // 1 megabyte + '\0'
#define MAX_OBJECTS 1000
#define MAX_CLASSES 1000
#define MAX_MEMBERS_PER_CLASS 100
#define MAX_METHODS_PER_CLASS 100
#define MAX_MEMBER_BYTES_PER_CLASS 10001
#define MAX_METHOD_BYTES_PER_CLASS 10001
#define MAX_WORDS_PER_METHOD 1000
#define MAX_DEFAULT_VALUE_LENGTH 251
// max length any single label can have in the parsed file (name of fcn, variable, class, method, arg, etc)
#define MAX_TOKEN_NAME_LENGTH 150
// max # of "DECLASS_H_" header files a single file being parsed can include
#define MAX_DECLASS_HEADER_FILES 300

// -:- COLA.C MACROS -:-
#define MAX_TOTAL_NUMBER_OF_FUNCTIONS_AND_MACROS_IN_PROGRAM 5000   // max # of macros & fcns parsed file can have
#define MAX_UNIQUE_OVERLOADS_PER_OVERLOADED_FCN_MACRO_INSTANCE 100 // max # of COLA's per overloaded fcn/macro name
#define MAX_LENGTH_PER_ARG 1001    // max # of characters per fcn arg list parsed file can have
#define MAX_ARGS_PER_FCN 251       // max # args per fcn arg list parsed file can have
#define BAD_CODE_BUFFER_LENGTH 251 // portion of code shown to user at error
/*****************************************************************************
 ||^\\ //=\\ //=\ /| |\ /\\  //\ /|==\ /\\ ||==== //^\\ ==== ==== //=\\ /\\ ||
 ||  ))|| || ||   || || ||\\//|| ||=   ||\\|| || |/===\| ||   ||  || || ||\\||
 ||_// \\=// \\=/ \\=// || \/ || \|==/ || \// || ||   || ||  ==== \\=// || \//
 *****************************************************************************
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
 *       (*) RATHER:   (1) include a c1 object as a member in c3, OR:       *
 *                     (2) mk c2 methods invoking c1 methods: c3 interface  *
 *   (9) ONLY BINARY SINGLE-LINE CONDITIONAL ("?:") OBJECT RETURNS:         *
 *       (*) IE NOT:   "return case ? obj : case2 ? obj2 : obj3;"           *
 *       (*) RATHER:   "if(case) return obj; return case2 ? obj2 : obj3;"   *
 *****************************************************************************
 *                       -:- DECLASS.C & SMRTPTR.H -:-                      *
 *    SMRTPTR.H LIBRARY IS DEFAULT INCLUDED, W/ IMPROVED MALLOC, CALLOC,    *
 *    REALLOC, FREE, & ASSERT FUNCTIONS AS WELL AS GARBAGE COLLECTION       *
 *      (*) "smrtptr.h" fcns same as stdlib/assert's all prefixed w/ "smrt" *
 *****************************************************************************
 *                        -:- DECLASS.C & COLA.C -:-                        *
 *    COLA.C (C OVERLOADED LENGTH ARGS) PARSER DFLT APPLIED PRIOR COMPILING *
 *      => ALLOWS FCN/MACRO POLYMORPHISM (2+ W/ SAME NAME) SO LONG AS THEY  *
 *         TAKE DIFFERENT NUMBERS OF ARGS AS PARAMETERS!                    *
 *           (*) thus "CTOR"s CAN be overloaded but "DTOR"s CANNOT          *
 *      => ALSO ENABLES DEFAULT FCN/METHOD ARG VALUES!                      *
 *      => "ODV" GUIDLINE BELOW HELPS AVOID OVERLOAD AMBIGUITY W/ DFLT VALS *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  *
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
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  *
 *  -:- ODV GUIDELINE TO COMBINE (O)VERLOADS W/ (D)EFAULT ARG (V)ALUES -:-  *
 * >> overload definitions must satisfy 1 of the following:                 *
 *    (1) an overload's # of non-dflt args must exceed the # of cumulative  *
 *        args (both dflt & not) of all other overloaded instances          *
 *    (2) an overload's # of cumulative args (both dflt & not) must be less *
 *        than the # of non-dflt args of all other overloaded instances     *
 *****************************************************************************
 *                     -:- DECLASS.C CTORS & DTORS -:-                      *
 *   FORMATTING:                                                            *
 *     (0) CONSTRUCTOR(CTOR) DENOTED AS TYPELESS METHOD W/ CLASS' NAME      *
 *         (*) can initialize contained object members as default value     *
 *         (*) user-invoked in obj declaration, auto-assigns defaults 1st   *
 *         (*) can take arguments                                           *
 *     (1) DESTRUCTOR(DTOR) DENOTED LIKE CTOR, BUT PREFIXED W/ '~':         *
 *         (*) container objs auto-dtor any member objs 1st in destruction  *
 *         (*) either invoked by user or autonomously once obj left scope   *
 *         (*) NEVER takes arguments                                        *
 *     (2) FOR BOTH CTORS & DTORS:                                          *
 *         (*) NEVER have a "return" value (being typeless)                 *
 *         (*) user can explicitly invoke (object "oName" & class "cName"): *
 *             => DTOR: destroy oName immediately: "~oName();"              *
 *             => CTOR: return a ctor'd cName obj instance: "cName(args);"  *
 *                => these so-called "dummy" ctors don't init specific objs *
 *                   rather they return a single instance of a ctor'd obj   *
 *                => "cName oName(args);" == "cName oName = cName(args);"   *
 *   DEFAULT PROPERTIES:                                                    *
 *     (0) DEFAULT CLASS CTOR/DTOR PROVIDED IF LEFT UNDEFINED BY USER       *
 *     (1) 3 KINDS OF OBJECTS NEVER DTOR'D IN THEIR IMMEDIATE SCOPE:        *
 *         (*) object args: dtor'd at the end of their invoking fcn's scope *
 *         (*) returned obj: assumed assigned as a val 2B dtor'd externally *
 *         (*) "immortal" obj: never dtor'd, see below to learn more        *
 *         => NOTE: THE LAST 2 ABOVE CAN BE DTOR'D W/ MACRO FLAGS 3-4 BELOW *
 *   OBJECT DECLARATIONS:                                                   *
 *     (0) SINGLE/ARRAY OBJ DEFAULT VALS:                                   *
 *         (*) SINGLE:   "className objectName;"                            *
 *         (*) ARRAY:    "className objectName[size];"                      *
 *     (1) SINGLE/ARRAY OBJ CTORS (AUTO-APPLIES DFLTS 1ST):                 *
 *         (*) SINGLE:   "className objectName(args);"                      *
 *         (*) ARRAY:    "className objectName[size](args);"                *
 *     (2) OBJ PTRS & INITIALIZING W/ CTOR/DFLT/NEITHER ("alloc" = any fcn) *
 *         (*) "className *objectName;" // "dangling" ptr, neither          *
 *         (*) "className *objectName(args);"           // ctor             *
 *         (*) "className *objectName(args) = alloc();" // ctor & dflt      *
 *     (3) OBJ PTR BEST PRACTICES TO REDUCE RISK OF ERRORS:                 *
 *         (*) DANGLING PTR: keep "immortal" unless class designed for ptrs *
 *         (*) NON-DANGLING: allocate memory & ctor upon declaration        *
 *****************************************************************************
 *                  -:- DECLASS.C & "immortal" KEYWORD -:-                  *
 *   (0) OBJECTS DECLARED "immortal" NEVER INVOKE THEIR DTOR; SEE (2) BELOW *
 *       (*) DECLARATION: "immortal className objName(args);"               *
 *       (*) contained objects can also be immortal                         *
 *       (*) WARNING: AN "IMMORTAL" OBJECT'S MEMBERS ALSO ALL = "IMMORTAL"  *
 *   (1) OBJ ARGS ALWAYS IMMORTAL: AS PLACEHOLDERS, THE PASSED OBJ THEY     *
 *       REPRESENT AREN'T OUT OF SCOPE ONCE FCN ENDS (NO DOUBLE DTOR)       *
 *       (*) thus denoting obj args as "immortal" is redundant              *
 *       (*) "MACRO FLAG" (3) disables all immortal obj EXCEPT obj args     *
 *   (2) IMMORTAL OBJECTS CAN ONLY BE DTOR'D IF EXPLICITLY BY THE USER, IE: *
 *       (*) immortal object "oName" only destroyed if "~oName();" invoked  *
 *****************************************************************************
 *                    -:- DECLASS.C 4 CMD LINE FLAGS -:-                    *
 *   (0) SHOW CLASS-OBJECT & COLA-OVERLOADING DATA:                         *
 *         (*) "-l": $ ./declass -l yourFile.c                              *
 *   (1) SAVE TEMP FILE MADE PRIOR TO PASSING CONVERTED FILE TO COLA.C:     *
 *         (*) "-save-temps": $ ./declass -save-temps yourFile.c            *
 *   (2) DON'T AUTO-COMPILE CONVERTED FILE (like #define DECLASS_NCOMPILE)  *
 *         (*) "-no-compile": $ ./declass -no-compile yourFile.c            *
 *   (3) MAKE FATAL ERRORS ASK USER WHETHER TO QUIT (RATHER THAN AUTOMATIC) *
 *         (*) DISCOURAGED last-resort way 2 debug, but errors 4 a reason!  *
 *         (*) "-mortal-errors": $ ./declass -mortal-errors yourFile.c      *
 *   ->> Combine any of the above, so long as "yourFile.c" is the last arg  *
 *         (*) VALID:   $ ./declass -no-compile -l -save-temps yourFile.c   *
 *         (*) INVALID: $ ./declass -no-compile -l yourFile.c -save-temps   *
 *****************************************************************************
 *               -:- DECLASS.C SPECIALIZATION MACRO FLAGS -:-               *
 *   BY PRECEDENCE:                                                         *
 *     (0) "#define DECLASS_IGNORE"       => DECLASS.C WON'T CONVERT FILE   *
 *     (1) "#define DECLASS_STRICTMODE"   => ENABLES MACRO FLAGS 2-4        *
 *     (2) "#define DECLASS_NSMRTPTR"     => DISABLES "SMRTPTR.H"           *
 *     (3) "#define DECLASS_NIMMORTAL"    => DISABLES "immortal" KEYWORD    *
 *     (4) "#define DECLASS_DTORRETURN"   => ALSO DTOR RETURNED OBJECTS     *
 *     (5) "#define DECLASS_NOISYSMRTPTR" => ALERT "SMRTPTR.H"'S ALLOC/FREE *
 *     (6) "#define DECLASS_NDEBUG"       => DISABLE "SMRTPTR.H" SMRTASSERT *
 *     (7) "#define DECLASS_NCOMPILE"     => ONLY CONVERT DON'T GCC COMPILE *
 *     (8) "#define DECLASS_NC11"         => GCC COMPILE W/O "-std=c11"     *
 *     (9) "#define DECLASS_NCOLA"        => DISABLE COLA.C OVERLOADS/DFLTS *
 *   DEFINING CUSTOM MEMORY ALLOCATION FUNCTIONS:                           *
 *     (0) declass.c relies on being able to identify memory allocation     *
 *         fcns to aptly apply dflt vals (not assigning garbage memory)     *
 *     (1) aside from malloc/calloc/smrtmalloc/smrtcalloc, users can        *
 *         declare their own custom class obj memory allocation functions   *
 *         to be recognized by declass.c at the top of their program        *
 *         (*) NOTE: ASSUMES ALL ALLOC FCNS RETURN "NULL" OR EXIT ON FAIL   *
 *     (2) list alloc fcn names after a "#define DECLASS_ALLOC_FCNS" macro  *
 *****************************************************************************
 *                     -:- DECLASS.C & HEADER FILES -:-                     *
 *   DENOTING HEADER FILES TO ALSO BE PARSED BY DECLASS.C:                  *
 *     (0) name must be prefixed w/ "DECLASS_H_" or "declass_h_"            *
 *     (1) name must contain the ".h" file extension                        * 
 *         (*) IE: #include "DECLASS_H_yourHeader.h"                        *
 *   IMPLEMENTATION:                                                        *
 *     (0) declass.c supports including header files that already contain   *
 *         classes (& arg # overloads/fcn dflt values if COLA not disabled) *
 *     (1) thus can write 1 class for several files, reduces redundant code *
 *     (2) as w/ a linker, header files are prepended to the main file to   *
 *         simulate as if user wrote a monolithic code base                 *
 *         (*) THUS HEADER FILES & MAIN FILE MUST CUMULATIVELY TAKE LESS    *
 *             MEMORY THAN THE "MAX_FILESIZE" MACRO!                        *
 *     (3) header files NOT prefixed "DECLASS_H_" or "declass_h_" will NOT  *
 *         be prepended to the main codebase by declass.c                   *
 *****************************************************************************/

// COLA.C STRUCTS

/* GLOBAL FILE FCN/MACRO & OVERLOADING TRACKING STRUCTURES */
// holds all global fcn & macro def's found in file
struct function_macro_instance {
  char name[MAX_TOKEN_NAME_LENGTH]; // defined fcn/macro name
  int args;            // number of args
  bool overloaded;     // whether its overloaded
  bool is_a_prototype; // whether is a function prototype
  bool is_a_macro;     // whether is a functionlike macro
} fmacs[MAX_TOTAL_NUMBER_OF_FUNCTIONS_AND_MACROS_IN_PROGRAM];
int fmacs_size = 0;

// holds function & macro overload instances, derived from "fmacs"
struct function_macro_overload_instance {
  char name[MAX_TOKEN_NAME_LENGTH]; // overoaded fcn/macro name
  int arg_sizes[MAX_UNIQUE_OVERLOADS_PER_OVERLOADED_FCN_MACRO_INSTANCE];   // different arg lengths per overload
  int arg_sizes_length; // # of other overoaded fcn/macros w/ same name
  bool is_a_macro[MAX_UNIQUE_OVERLOADS_PER_OVERLOADED_FCN_MACRO_INSTANCE]; // whether each overload is a functionlike macro
} overload_fmacs[MAX_TOTAL_NUMBER_OF_FUNCTIONS_AND_MACROS_IN_PROGRAM];
int overload_fmacs_size = 0;

/* GLOBAL FILE FCN-WITH-DEFAULT-VALUE-ARGS TRACKING STRUCTURES */
// holds all global fcn def's found in file with default arg values
struct function_with_default_value_instance {
  char fcn_name[MAX_TOKEN_NAME_LENGTH]; // fcn instance name
  int total_args;    // fcn's total args (both dflt & not)
  int total_dflts;   // fcn's total dflt args
  int dflt_idxs[MAX_ARGS_PER_FCN]; // fcn's dflt arg idxs wrt its arg list
  char dflt_vals[MAX_ARGS_PER_FCN][MAX_LENGTH_PER_ARG];      // fcn's dflt values
} all_dflt_fcns[MAX_TOTAL_NUMBER_OF_FUNCTIONS_AND_MACROS_IN_PROGRAM]; // struct to hold all dflt-valued fcn instances
int all_dflt_fcns_size = 0; // total functions registered

// holds defaulted-arg function instances, derived from "all_dflt_fcns"
struct unique_function_default_values {
  char name[MAX_TOKEN_NAME_LENGTH]; // defaulted-arg (& possibly overoaded) fcn name
  int arg_sizes_length; // # of other overoaded fcns w/ same name
  int arg_sizes[MAX_UNIQUE_OVERLOADS_PER_OVERLOADED_FCN_MACRO_INSTANCE];   // arg length(s) (> 1 if overloaded)
  int total_dflts[MAX_UNIQUE_OVERLOADS_PER_OVERLOADED_FCN_MACRO_INSTANCE]; // total arg dflt(s) (> 1 if overloaded)
  int dflt_idxs[MAX_UNIQUE_OVERLOADS_PER_OVERLOADED_FCN_MACRO_INSTANCE][MAX_ARGS_PER_FCN]; // arg dflt idx(s) (> 1 if overloaded)
  char dflt_vals[MAX_UNIQUE_OVERLOADS_PER_OVERLOADED_FCN_MACRO_INSTANCE][MAX_ARGS_PER_FCN][MAX_LENGTH_PER_ARG]; // arg dflt val(s) (> 1 if overloaded)
} unique_dflt_fcns[MAX_TOTAL_NUMBER_OF_FUNCTIONS_AND_MACROS_IN_PROGRAM];   // dflt-arg fcn instance
int unique_dflt_fcns_size = 0; // total unique fcn names associated w/ having 1+ dflt args


// DECLASS.C STRUCTS

// stores class names, & their associated methods
struct class_info { 
  char class_name[MAX_TOKEN_NAME_LENGTH], method_names[MAX_METHODS_PER_CLASS][MAX_TOKEN_NAME_LENGTH]; 
  char member_names[MAX_MEMBERS_PER_CLASS][MAX_TOKEN_NAME_LENGTH], member_values[MAX_MEMBERS_PER_CLASS][MAX_DEFAULT_VALUE_LENGTH];
  int  class_uctor_arg_lengths[MAX_METHODS_PER_CLASS]; // # of args per user-defined ctor (helps create unique dummy/array ctors per COLA overload)
  bool class_has_ctor, class_has_ctor_args;            // class has user-defined ctor to invoke when assigning default
  bool class_has_dtor;                                 // class has user-defined dtor to invoke when leaving obj scope
  bool class_has_alloc;                                // class has 1+ member of: malloc/calloc/smrtmalloc/smrtcalloc
  bool member_is_array[MAX_MEMBERS_PER_CLASS];         // init empty arrays as {0}
  bool member_is_pointer[MAX_MEMBERS_PER_CLASS];       // init pointers as 0 (same as NULL)
  bool member_value_is_alloc[MAX_MEMBERS_PER_CLASS];   // track alloc'd members for '-l' awareness & "sizeof()" arg
  bool member_is_immortal[MAX_MEMBERS_PER_CLASS];      // track "immortal" members: never trigger user-def'd dtors (unless explicitly invoked)
  char member_value_user_ctor[MAX_MEMBERS_PER_CLASS][MAX_TOKEN_NAME_LENGTH];   // track user-define ctor arr vals (spliced out for DFLT fcn)
  char member_object_class_name[MAX_MEMBERS_PER_CLASS][MAX_TOKEN_NAME_LENGTH]; // used to intialize contained class objects
  int total_methods, total_members, total_uctors;
} classes[MAX_CLASSES];
int total_classes = 0;

// stores object names, & their associated class
struct objNames { 
  char class_name[MAX_TOKEN_NAME_LENGTH], object_name[MAX_TOKEN_NAME_LENGTH]; 
  bool is_class_pointer, is_alloced_class_pointer, is_class_array;
  bool class_has_dtor, is_immortal, is_dangling_ctord_ptr;
} objects[MAX_OBJECTS];
int total_objects = 0;

// keeps track of "#define"'d flags in the user's program to alter the interpreter's course
#define TOTAL_FLAGS 6
struct user_hashtag_defined_flags {
  char flags[TOTAL_FLAGS][30];
  bool defaults[TOTAL_FLAGS];
  bool non_dflt[TOTAL_FLAGS];
} DEFNS = {
  {
    "DECLASS_IGNORE",    "DECLASS_STRICTMODE", "DECLASS_NSMRTPTR",
    "DECLASS_NIMMORTAL", "DECLASS_DTORRETURN", "DECLASS_NOISYSMRTPTR"
  },
  {false, false, true,  true,  false, false}, 
  {true,  true,  false, false, true,  true}
};
// flags ordered by superseding precedence, & in same order as their "DEFNS.flags[][]" counterparts
bool *NO_DECLASS    = &DEFNS.defaults[0];  // file is not to be interpretted - terminate program    (default false)
bool *STRICT_MODE   = &DEFNS.defaults[1];  // !SMRT_PTRS && !IMMORTALITY && DTOR_RETURN             (default false)
bool *SMRT_PTRS     = &DEFNS.defaults[2];  // confirms default inclusion of smrtptr.h               (default true)
bool *IMMORTALITY   = &DEFNS.defaults[3];  // confirms default enabling of "immortal" keyword       (default true)
bool *DTOR_RETURN   = &DEFNS.defaults[4];  // returned objects also dtor'd                          (default false)
bool *NOISY_SMRTPTR = &DEFNS.defaults[5];  // confirms whether to alert all smrtptr.h alloc/freeing (default false)
bool NO_SMRTASSERT       = false;          // deactivates all "smrtassert()" statements             (default false)
bool NO_C11_COMPILE_FLAG = false;          // compiles declassified file w/o "-std=c11"             (default false)
bool NO_COMPILE          = false;          // declass.c declassifies but DOESN'T compile given file (default false)
bool NO_COLA_PARSER      = false;          // prevents passing converted file to cola.c overloader  (default false)
bool MORTAL_ERRORS       = false;          // fatal errors will ask user whether to quit/continue   (default false)

/* NOTE: IT IS ASSUMED THAT USER-DEFINED ALLOCATION FCNS RETURN NULL OR END PROGRAM UPON ALLOC FAILURE */
int TOTAL_ALLOC_FCNS = 4; // increases if user defines their own allocation fcns
// user can add up to 100 of their own alloc fcns
char ALLOC_FCNS[104][MAX_TOKEN_NAME_LENGTH * 2] = { "malloc", "calloc", "smrtmalloc", "smrtcalloc" };

// cumulative filesizes of the main program & its #included "DECLASS_H_" header files
long long CUMULATIVE_FILESIZE = 0;
// array of the "DECLASS_H_" header files included.
// name length is + 8 to account for "delass_" prefix
char DECLASS_HEADER_FILE_NAMES[MAX_DECLASS_HEADER_FILES][MAX_TOKEN_NAME_LENGTH + 8];
int TOTAL_DECLASS_HEADER_FILES = 0;

// basic c type keywords:
#define TOTAL_TYPES 14
char basic_c_types[TOTAL_TYPES][11] = {
  "char ","short ","int ","unsigned ","signed ","struct ","union ",
  "long ","float ","double ","bool ","enum ","typedef ","void "
};
// brace-additions
#define TOTAL_BRACE_KEYWORDS 5
char brace_keywords[TOTAL_BRACE_KEYWORDS][8] = {"else if", "if", "else", "for", "while"};

/* C.O.L.A. "C OVERLOADED LENGTH ARGUMENTS" PARSER MAIN EXECUTION */
bool COLA_C_main_execution(bool, char*);
/* BRACE-ADDITION FUNCTION */
bool at_smrtassert_or_compile_or_cola_macro_flag(char*);
void add_braces(char []);
/* "DECLASS_H_" HEADER FILE LINKING FUNCTIONS */
void scrape_and_include_all_DECLASS_H_headers(char*);
bool is_valid_DECLASS_H_header_file(char*);
bool is_unique_DECLASS_H_header(char*);
void parse_file_for_DECLASS_H_header_files(char*);
/* MESSAGE FUNCTIONS */
void get_invalid_code_snippet(const char*, const char*, char []); // defined in COLA.C below
void process_cmd_flag(char*, bool*, bool*);
void enable_smrtptr_alerts();
void confirm_no_reserved_DC_prefix(char*);
void confirm_valid_file(char*);
void confirm_command_processor_exists_for_autonomous_compilation(int);
void declass_DECLASSIFIED_ascii_art();
void declass_ERROR_ascii_art();
void declass_missing_Cfile_alert();
void throw_DECLASS_IGNORE_message_and_terminate();
void show_l_flag_data();
void ask_user_whether_to_quit_or_continue_declassification();
void POSSIBLE_DUMMY_CTOR_METHOD_ARG_ERROR_MESSAGE(const char [12],int,int);
void throw_potential_invalid_double_dflt_assignment(const char [12], int, char*);
void throw_fatal_error_dtor_has_args(char*, int, char*);
/* USER-DEFINED ALLOCATION FUNCTIONS PARSING/REGISTERING FUNCTIONS */
bool is_an_alloc_fcn(char*);
void register_user_defined_alloc_fcns(char*);
/* COMMENT & BLANK LINE SKIPPING FUNCTIONS */
void whitespace_all_comments(char*);
void trim_sequential_spaces(char []);
int remove_blank_lines(char*);
/* STRING HELPER FUNCTIONS */
bool no_overlap(char, char*);
bool is_at_substring(char*, char*);
void account_for_string_char_scopes(bool*, bool*, bool*, char*);
/* DUMMY CTOR OBJECT INVOCATION CHAIN PARSER */
void get_dummy_ctor_invoking_object_chain(char*, char*, int, bool*, bool*);
/* OBJECT CONSTRUCTION FUNCTIONS */
bool is_a_dummy_ctor(char*);
void mk_initialization_brace(char [], int);
void mk_object_array_ctor_macros(char [], char*);
void mk_ctor_macros(char [], char*);
void mk_class_global_initializer(char*, char*, char*);
int prefix_dummy_ctor_with_DC__DUMMY_(char*, char*);
void mk_dummy_ctor_macros(char*, char*);
/* USER-DEFINED OBJECT CONSTRUCTOR (CTOR) PARSING FUNCTIONS */
bool get_user_ctor(char*, char*, char*, bool*);
char *check_for_ctor_obj(char*, char*, int*, int*, bool*);
/* OBJECT ARRAY DESTRUCTION-MACRO CREATION FUNCTION */
void mk_dtor_array_macro(char [], char*);
/* USER-DEFINED OBJECT DESTRUCTOR (DTOR) PARSING & SPLICING FUNCTIONS */
int shiftSplice_dtor_in_buffer(char*, char*, bool);
bool object_is_returned(char*);
void get_if_else_object_idxs(char*, int*);
bool unique_dtor_condition(char*, char*);
int one_line_conditional(char*, bool);
int return_then_immediate_exit(char*);
void add_object_dtor(char*, bool, bool);
/* OBJECT METHOD PARSER */
int parse_method_invocation(char*, char*, int*, bool, char[][MAX_TOKEN_NAME_LENGTH]);
void splice_in_prepended_method_name(char*, char*, int, char*, int*, int*, char[][MAX_TOKEN_NAME_LENGTH]);
void splice_in_prepended_NESTED_method_name(char*, char*, int, char*, int*, char[][MAX_TOKEN_NAME_LENGTH]);
void rmv_excess_buffer_objectChain(char*, char*, char*, char*, char*);
/* OBJECT METHOD PARSER HELPER FUNCTIONS */
int is_method_invocation(char*);
bool invoked_member_is_method(char*, char*, bool);
void get_invoked_member_name(char*, char*);
void get_object_name(char*, char*, char*, int, char [][MAX_TOKEN_NAME_LENGTH], bool, bool*);
int prefix_local_members_and_cpy_method_args(char*, char*, char [][MAX_TOKEN_NAME_LENGTH], int*, char);
/* STORE OBJECT INFORMATION */
bool store_object_info(char*, int, bool*);
/* PARSE CLASS HELPER FUNCTIONS */
void initialize_new_class_default_properties(char*);
bool is_struct_definition(char*);
void get_class_name(char*, char*);
void confirm_only_one_cdtor(char*, char*, bool);
void confirm_only_valid_cola_overloads(char*);
int count_class_cdtor_args(char*);
void get_prepended_method_name(char*, char*, char*, bool*, bool*);
int get_initialized_member_value(char*);
void register_member_class_objects(char*);
void check_for_alloc_sizeof_arg();
int get_class_member(char*, bool);
void add_method_word(char [][MAX_TOKEN_NAME_LENGTH], int*, char*, char*);
/* CONFIRM WHETHER METHOD'S WORD IS A LOCAL CLASS MEMBER */
bool not_local_var_declaration(char*);
bool not_in_method_words(char [][MAX_TOKEN_NAME_LENGTH], int, char*);
bool not_an_external_invocation(char*);
void splice_in_this_arrowPtr(char*);
int parse_local_nested_method(char*, char*, char*, char [][MAX_TOKEN_NAME_LENGTH]);
bool valid_member(char*, char*, char, char, char [][MAX_TOKEN_NAME_LENGTH], int);
/* PARSE CLASS */
int parse_class(char*, char [], int*);

// declassed program contact header, "immortal" keyword, & deactivate smrtassert
#define DC_SUPPORT_CONTACT "Email jrandleman@scu.edu or see https://github.com/jrandleman for support */\nint DC__NDTR = 1;"
#define IMMORTAL_KEYWORD_DEF "#define immortal // immortal keyword active\n"
#define NDEBUG_SMRTPTR_DEF "#define DECLASS_NDEBUG // smartptr.h \"smrtassert()\"statements disabled\n"
// smrtptr.h to implement stdlib.h's memory handling functions w/ garbage collection
char DC_SMART_POINTER_H_[5000] = "\
/****************************** SMRTPTR.H START ******************************/\n\
// Source: https://github.com/jrandleman/C-Libraries/tree/master/Smart-Pointer\n\
#ifndef SMRTPTR_H_\n\
#define SMRTPTR_H_\n\
#include <stdio.h>\n\
#include <stdlib.h>\n\
// garbage collector & smart pointer storage struct\n\
static struct SMRTPTR_GARBAGE_COLLECTOR {\n\
  long len, max; // current # of ptrs && max capacity\n\
  void **ptrs;   // unique ptr set to free all smrt ptrs\n\
} SMRTPTR_GC = {-1};\n\
// invoked by atexit to free all ctor-alloc'd memory\n\
static void smrtptr_free_all() {\n\
  int i = 0;\n\
  for(; i < SMRTPTR_GC.len; ++i) free(SMRTPTR_GC.ptrs[i]);\n\
  if(SMRTPTR_GC.len > 0) free(SMRTPTR_GC.ptrs);\n\
  // if(SMRTPTR_GC.len > 0) printf(\"FREED %ld SMART POINTERS!\\n\", SMRTPTR_GC.len); // optional\n\
  SMRTPTR_GC.len = 0;\n\
}\n\
// throws invalid allocation errors\n\
static void smrtptr_throw_bad_alloc(char *alloc_type, char *smrtptr_h_fcn) {\n\
  fprintf(stderr, \"\\n-:- \\033[1m\\033[31mERROR\\033[0m COULDN'T %s MEMORY FOR SMRTPTR.H'S %s -:-\\n\\n\", alloc_type, smrtptr_h_fcn);\n\
  fprintf(stderr, \"-:- FREEING ALLOCATED MEMORY THUS FAR AND TERMINATING PROGRAM -:-\\n\\n\");\n\
  exit(EXIT_FAILURE); // still frees any ptrs allocated thus far\n\
}\n\
// acts like assert, but exits rather than abort to free smart pointers\n\
#ifndef DECLASS_NDEBUG\n\
#define smrtassert(condition) ({\\\n\
  if(!(condition)) {\\\n\
    fprintf(stderr, \"\\n\\033[1m\\033[31mERROR\\033[0m Smart Assertion failed: (%s), function %s, file %s,\
 line %d.\\n\", #condition, __func__, __FILE__, __LINE__);\\\n\
    fprintf(stderr, \">> Freeing Allocated Smart Pointers & Terminating Program.\\n\\n\");\\\n\
    exit(EXIT_FAILURE);\\\n\
  }\\\n\
})\n\
#else\n\
#define smrtassert(condition)\n\
#endif\n\
// smrtptr stores ptr passed as arg to be freed atexit\n\
void smrtptr(void *ptr) {\n\
  // free ptrs atexit\n\
  atexit(smrtptr_free_all);\n\
  // malloc garbage collector\n\
  if(SMRTPTR_GC.len == -1) {\n\
    SMRTPTR_GC.ptrs = malloc(sizeof(void *) * 10);\n\
    if(!SMRTPTR_GC.ptrs) {\n\
      fprintf(stderr, \"\\n-:- \\033[1m\\033[31mERROR\\033[0m COULDN'T MALLOC MEMORY TO INITIALIZE SMRTPTR.H'S GARBAGE COLLECTOR -:-\\n\\n\");\n\
      exit(EXIT_FAILURE);\n\
    }\n\
    SMRTPTR_GC.max = 10, SMRTPTR_GC.len = 0;\n\
  }\n\
  // reallocate if \"max\" ptrs already added\n\
  if(SMRTPTR_GC.len == SMRTPTR_GC.max) {\n\
    SMRTPTR_GC.max *= SMRTPTR_GC.max;\n\
    void **SMRTPTR_TEMP = realloc(SMRTPTR_GC.ptrs, sizeof(void *) * SMRTPTR_GC.max);\n\
    if(!SMRTPTR_TEMP) smrtptr_throw_bad_alloc(\"REALLOC\", \"GARBAGE COLLECTOR\");\n\
    SMRTPTR_GC.ptrs = SMRTPTR_TEMP;\n\
  }\n\
  // add ptr to SMRTPTR_GC if not already present (ensures no double-freeing)\n\
  int i = 0;\n\
  for(; i < SMRTPTR_GC.len; ++i) if(SMRTPTR_GC.ptrs[i] == ptr) return;\n\
  SMRTPTR_GC.ptrs[SMRTPTR_GC.len++] = ptr;\n\
  // printf(\"SMART POINTER #%ld STORED!\\n\", SMRTPTR_GC.len); // optional\n\
}\n\
// malloc's a pointer, stores it in the garbage collector, then returns ptr\n\
void *smrtmalloc(size_t alloc_size) {\n\
  void *smtr_malloced_ptr = malloc(alloc_size);\n\
  if(smtr_malloced_ptr == NULL) smrtptr_throw_bad_alloc(\"MALLOC\", \"SMRTMALLOC FUNCTION\");\n\
  smrtptr(smtr_malloced_ptr);\n\
  return smtr_malloced_ptr;\n\
}\n\
// calloc's a pointer, stores it in the garbage collector, then returns ptr\n\
void *smrtcalloc(size_t alloc_num, size_t alloc_size) {\n\
  void *smtr_calloced_ptr = calloc(alloc_num, alloc_size);\n\
  if(smtr_calloced_ptr == NULL) smrtptr_throw_bad_alloc(\"CALLOC\", \"SMRTCALLOC FUNCTION\");\n\
  smrtptr(smtr_calloced_ptr);\n\
  return smtr_calloced_ptr;\n\
}\n\
// realloc's a pointer, stores it anew in the garbage collector, then returns ptr\n\
// compatible both \"smart\" & \"dumb\" ptrs!\n\
void *smrtrealloc(void *ptr, size_t realloc_size) {\n\
  int i = 0;\n\
  void *smtr_realloced_ptr;\n\
  // realloc a \"smart\" ptr already in garbage collector\n\
  for(; i < SMRTPTR_GC.len; ++i)\n\
    if(SMRTPTR_GC.ptrs[i] == ptr) {\n\
      smtr_realloced_ptr = realloc(ptr, realloc_size); // frees ptr in garbage collector\n\
      if(smtr_realloced_ptr == NULL) smrtptr_throw_bad_alloc(\"REALLOC\", \"SMRTREALLOC FUNCTION\");\n\
      SMRTPTR_GC.ptrs[i] = smtr_realloced_ptr; // point freed ptr at realloced address\n\
      // printf(\"SMART POINTER REALLOC'D!\\n\"); // optional\n\
      return smtr_realloced_ptr;\n\
    }\n\
  // realloc a \"dumb\" ptr then add it to garbage collector\n\
  smtr_realloced_ptr = realloc(ptr, realloc_size);\n\
  if(smtr_realloced_ptr == NULL) smrtptr_throw_bad_alloc(\"REALLOC\", \"SMRTREALLOC FUNCTION\");\n\
  smrtptr(smtr_realloced_ptr);\n\
  return smtr_realloced_ptr;\n\
}\n\
// prematurely frees ptr arg prior to atexit (if exists)\n\
void smrtfree(void *ptr) {\n\
  int i = 0, j;\n\
  for(; i < SMRTPTR_GC.len; ++i) // find ptr in garbage collector\n\
    if(SMRTPTR_GC.ptrs[i] == ptr) {\n\
      free(ptr);\n\
      for(j = i; j < SMRTPTR_GC.len - 1; ++j) // shift ptrs down\n\
        SMRTPTR_GC.ptrs[j] = SMRTPTR_GC.ptrs[j + 1];\n\
      SMRTPTR_GC.len--;\n\
      // printf(\"SMART POINTER FREED!\\n\"); // optional\n\
      return;\n\
    }\n\
}\n\
#endif\n\
/******************************* SMRTPTR.H END *******************************/";

/******************************************************************************
* MAIN EXECUTION
******************************************************************************/

int main(int argc, char *argv[]) {
  // confirm passed .c file in cmd line arg to declass
  if(argc<2 || argc>6 || argv[argc-1][strlen(argv[argc-1])-2] != '.' || 
    (argv[argc-1][strlen(argv[argc-1])-1] != 'c' && argv[argc-1][strlen(argv[argc-1])-1] != 'C'))
    declass_missing_Cfile_alert();
  // process cmd line flags: 1) determine if displaying class info at exit: "-l"
  //                         2) determine if not to compile: "-no-compile"
  //                         3) determine if to save temp "pre-COLA" file: "-save-temps"
  bool show_class_info = false, save_temp_files = false;
  if(argc > 2)
    for(int i = 1; i < argc - 1; ++i)
      process_cmd_flag(argv[i], &show_class_info, &save_temp_files);

  // old & new file buffers, as well as filename
  char file_contents[MAX_FILESIZE], NEW_FILE[MAX_FILESIZE];
  char filename[MAX_TOKEN_NAME_LENGTH], original_filename_executable[MAX_TOKEN_NAME_LENGTH];
  FLOOD_ZEROS(file_contents, MAX_FILESIZE); FLOOD_ZEROS(NEW_FILE, MAX_FILESIZE);
  FLOOD_ZEROS(filename, MAX_TOKEN_NAME_LENGTH); FLOOD_ZEROS(original_filename_executable, MAX_TOKEN_NAME_LENGTH);
  strcpy(filename, argv[argc-1]);
  confirm_valid_file(filename);
  strcpy(original_filename_executable, filename);
  NEW_EXTENSION(original_filename_executable, ""); // remove ".c" from executable file's name

  FSCRAPE(file_contents, filename);
  char filler_array_argument[MAX_WORDS_PER_METHOD][MAX_TOKEN_NAME_LENGTH];
  int i = 0, j = 0;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  bool found_COLA_overloads = false;

  char dummy_ctor_invoking_obj_chain[MAX_TOKEN_NAME_LENGTH * 2], *read, *write;
  bool dummy_ctor_ptr_invoked, dummy_ctor_invoker_redefined;
  int dummy_ctor_len;
  FLOOD_ZEROS(dummy_ctor_invoking_obj_chain, MAX_TOKEN_NAME_LENGTH * 2);

  // remove commments from program: ensures braces applied properly & reduces size
  whitespace_all_comments(file_contents);
  trim_sequential_spaces(file_contents);
  // confirm user didn't use "DC_" to prefix any token names
  confirm_no_reserved_DC_prefix(file_contents);
  // check for & scrape any "#include"'d "DECLASS_H_" header files & prepend them to "file_contents"
  scrape_and_include_all_DECLASS_H_headers(file_contents);
  // wrap braces around single-line "braceless" if, else if, else, while, & for loops
  add_braces(file_contents);
  // simulate as if "#define DECLASS_NCOMPILE" were found if command processor DNE
  confirm_command_processor_exists_for_autonomous_compilation(__LINE__);
  // uncomment smrtptr.h's alerts if user included "#define DECLASS_NOISYSMRTPTR"
  if(*SMRT_PTRS && *NOISY_SMRTPTR) enable_smrtptr_alerts();

  while(file_contents[i] != '\0') {
    // don't modify anything in strings
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, &file_contents[i]);

    // store class info & convert to structs calling external fcns
    if(in_token_scope && is_at_substring(&file_contents[i], "class ") && (!VARCHAR(file_contents[i-1]))) 
      i += parse_class(&file_contents[i], NEW_FILE, &j);

    // store declared class object info
    bool dummy_ctor = false;
    for(int k = 0; in_token_scope && k < total_classes; ++k)
      if(is_at_substring(&file_contents[i], classes[k].class_name) 
        && !VARCHAR(file_contents[i+strlen(classes[k].class_name)]) && !VARCHAR(file_contents[i-1]))
        if(store_object_info(&file_contents[i], 0, &dummy_ctor)) { // assign default values

          // check if a so-called "dummy ctor" was detected and splice 
          // in the "DC__DUMMY_" class/ctor name's prefix if so
          if(dummy_ctor) {
            read = &file_contents[i], write = &NEW_FILE[j], dummy_ctor_ptr_invoked = dummy_ctor_invoker_redefined = false;
            FLOOD_ZEROS(dummy_ctor_invoking_obj_chain, MAX_TOKEN_NAME_LENGTH * 2);
            // flag the object to be destroyed again after being assigned to a dummy ctor (being that they're a new obj instance)
            get_dummy_ctor_invoking_object_chain(read, dummy_ctor_invoking_obj_chain, k, &dummy_ctor_ptr_invoked, &dummy_ctor_invoker_redefined);
            dummy_ctor_len = prefix_dummy_ctor_with_DC__DUMMY_(write, read);
            j = strlen(NEW_FILE), i += dummy_ctor_len;
            // As per Documentation's "DECLASS.C CTORS & DTORS" > "FORMATTING" > "(2)", dummy ctors only 
            // return single obj instances, thus "obj arrays" are presumed to be assigning one of their
            // individual object "cells" w/in.
            add_object_dtor(&file_contents[i], true, dummy_ctor_invoker_redefined); 
            if(dummy_ctor_invoker_redefined) { // Don't add for new declarations
              if(dummy_ctor_ptr_invoked)
                sprintf(&NEW_FILE[j], ";if(%s){%s->DC_DTR=&DC__NDTR;}", dummy_ctor_invoking_obj_chain, dummy_ctor_invoking_obj_chain);
              else if(dummy_ctor_invoker_redefined)
                sprintf(&NEW_FILE[j], ";%s.DC_DTR=&DC__NDTR", dummy_ctor_invoking_obj_chain);
              j = strlen(NEW_FILE);
            }
            break;
          }

          // check if class object is already initialized by user
          int already_assigned = i;
          while(file_contents[already_assigned] != '\0' && no_overlap(file_contents[already_assigned], "\n;,=")) 
            ++already_assigned;
          if(file_contents[already_assigned] == '=' && !objects[total_objects-1].is_alloced_class_pointer) {
            add_object_dtor(&file_contents[i], true, false); 
            break;
          }

          // determine if object is invoking it's user-defined constructor
          char *user_ctor_finder = &file_contents[i], user_ctor[1000];
          FLOOD_ZEROS(user_ctor, 1000);
          bool is_fcn_returning_obj = false;
          bool user_ctor_invoked = get_user_ctor(user_ctor_finder,user_ctor,classes[k].class_name,&is_fcn_returning_obj);

          // don't splice in any constructors if "object" is actually a fcn returning an object
          if(is_fcn_returning_obj) break;

          // initialization undefined -- use default initial values
          while(file_contents[i] != '\0' && file_contents[i-1] != ';') NEW_FILE[j++] = file_contents[i++];
          // if an obj ptr allocating memory, confirm obj != NULL prior to passing to ctors & dflt-val assignment
          objects[total_objects-1].is_dangling_ctord_ptr = false; // ID dangling ptrs (no ctor/'='/alloc) to assign "objPtr=NULL" in "add_object_dtor()"
          if(objects[total_objects-1].is_alloced_class_pointer) {
            sprintf(&NEW_FILE[j], " if(%s){", objects[total_objects-1].object_name), j = strlen(NEW_FILE);
          // if obj a ctor'd ptr but not alloc'd, still assign dangling ptr to NULL prior any CTOR's
          } else if(objects[total_objects-1].is_class_pointer) {
            sprintf(&NEW_FILE[j], " %s=NULL;", objects[total_objects-1].object_name), j = strlen(NEW_FILE);
            objects[total_objects-1].is_dangling_ctord_ptr = true;
          }
          // only apply default values if either a non-ptr or an allocated ptr
          if(!objects[total_objects-1].is_class_pointer || objects[total_objects-1].is_alloced_class_pointer) {
            if(objects[total_objects-1].is_class_array)        // object = array, use macro init
              sprintf(&NEW_FILE[j], " DC__%s_ARR(%s);", classes[k].class_name, objects[total_objects-1].object_name);
            else if(objects[total_objects-1].is_class_pointer) // object != array, so init via its class' global object & init its "2-Be_Dtor'd" flag
              sprintf(&NEW_FILE[j], " DC__%s_CTOR((*%s));%s->DC_DTR=&DC__NDTR;", 
                classes[k].class_name, objects[total_objects-1].object_name, objects[total_objects-1].object_name);
            else                                               // object != array, so init via its class' global object
              sprintf(&NEW_FILE[j], " DC__%s_CTOR(%s);", classes[k].class_name, objects[total_objects-1].object_name);
            j = strlen(NEW_FILE);
          }

          // add user-defined ctor invocation w/ initialization values (if present)
          if(user_ctor_invoked) { sprintf(&NEW_FILE[j], " %s", user_ctor); j = strlen(NEW_FILE); }
          add_object_dtor(&file_contents[i], false, false); // splice in object's class dtor at the end of the current scope

          // if an obj ptr allocing memory, close the "if != null" braced-condition
          if(objects[total_objects-1].is_alloced_class_pointer) NEW_FILE[j++] = '}';
          break;
        }

    // modify object invoking method to fcn call w/ a prepended class-converted-struct name
    if(in_token_scope && total_classes > 0) 
      i += parse_method_invocation(&file_contents[i], NEW_FILE, &j, false, filler_array_argument);

    // save non-class data to file
    NEW_FILE[j++] = file_contents[i++]; 
  }
  NEW_FILE[j] = '\0';


  // finishing touches -- remove spaces, write to file the struct / methods-turned-fcns / general file 
  // buffers, pass to "cola.c", output class object details as per "-l" flag, & GCC compile as needed.
  trim_sequential_spaces(NEW_FILE);

  char HEADED_NEW_FILE[MAX_FILESIZE]; FLOOD_ZEROS(HEADED_NEW_FILE, MAX_FILESIZE);

  // determine if ought to include smrtptr.h/"immortal"-keyword at top of file - as per whether
  // "#define DECLASS_NSMRTPTR" or "#define DECLASS_NIMMORTAL" wasn't/was found
  char *headed_new_file_ptr = HEADED_NEW_FILE;
  sprintf(headed_new_file_ptr,"/* DECLASSIFIED: %s\n * %s\n", filename, DC_SUPPORT_CONTACT);
  headed_new_file_ptr += strlen(headed_new_file_ptr);
  if(*IMMORTALITY)   sprintf(headed_new_file_ptr,"%s", IMMORTAL_KEYWORD_DEF); // include "immortal" keyword if active
  headed_new_file_ptr += strlen(headed_new_file_ptr);
  if(NO_SMRTASSERT) sprintf(headed_new_file_ptr,"%s", NDEBUG_SMRTPTR_DEF);    // disable smrtptr.h smrtassert() if active
  headed_new_file_ptr += strlen(headed_new_file_ptr);
  if(*SMRT_PTRS)     sprintf(headed_new_file_ptr,"%s", DC_SMART_POINTER_H_);  // include smrtptr.h if active
  headed_new_file_ptr += strlen(headed_new_file_ptr);
  sprintf(headed_new_file_ptr,"\n\n%s", NEW_FILE);


  // notify user declassification conversion completed
  declass_DECLASSIFIED_ascii_art();
  printf("%s \033[1m==DECLASSIFIED=>\033[0m ", filename);
  if(NO_COLA_PARSER) 
    NEW_EXTENSION(filename, "_DECLASS.c");
  else
    NEW_EXTENSION(filename, "_DECLASS_PRECOLA.c");
  printf("%s", filename);
  if(show_class_info || !NO_COLA_PARSER || NO_COMPILE) 
    printf("\n=================================================================================");
  if(show_class_info || !NO_COLA_PARSER) printf("\n");

  // write newly converted/declassified file & notify user as to whether passing to "COLA.C" or not
  FPUT(HEADED_NEW_FILE,filename);

  // if passing to "cola.c"
  if(!NO_COLA_PARSER) {
    printf("\033[1m >> RUNNING COLA.C:\033[0m\n=================================================================================\n");
    found_COLA_overloads = COLA_C_main_execution(show_class_info, filename);
    if(!save_temp_files && found_COLA_overloads) remove(filename); // remove temporary "precola" file if overloads were found
  } 

  // if compiling & cola-overloaded
  if(!NO_COLA_PARSER && !NO_COMPILE) {
    // cola.c creates a new file w/ the original filename + "_DECLASS.c" on success
    FLOOD_ZEROS(filename, MAX_TOKEN_NAME_LENGTH);
    sprintf(filename, "%s_DECLASS.c", original_filename_executable);
  }

  // show "-l" flag class info & (if applicable/able) compile program
  if(show_class_info) show_l_flag_data();

  // generate compilation cmd
  char compile_cmd[(MAX_TOKEN_NAME_LENGTH * 2) + 10];
  FLOOD_ZEROS(compile_cmd, (MAX_TOKEN_NAME_LENGTH * 2) + 10);
  if(!NO_COMPILE && NO_C11_COMPILE_FLAG)
    sprintf(compile_cmd, "gcc -o %s %s", original_filename_executable, filename);
  else if(!NO_COMPILE)
    sprintf(compile_cmd, "gcc -std=c11 -o %s %s", original_filename_executable, filename);
  if(!NO_COMPILE) {
    printf("\n=================================================================================\n");
    printf("%s \033[1m=GCC=COMPILES=TO=>\033[0m %s", filename, original_filename_executable);
    printf("\n=================================================================================\n");
  }
  if(!NO_COMPILE) { 
    printf("\033[1mCOMPILING CONVERTED CODE:\033[0m\n  $ %s", compile_cmd);
    printf("\n=================================================================================\n");
    system(compile_cmd); // compile the declassified/converted code
  }
  
  // Based on prior '=' sign divider output
  if((NO_COMPILE && show_class_info) || (NO_COMPILE && !show_class_info && !found_COLA_overloads && !NO_COLA_PARSER)) 
    printf("\n=============================\n");
  else if(NO_COMPILE && !show_class_info && !found_COLA_overloads && NO_COLA_PARSER)
    printf("\n");
  printf(" >> Terminating Declassifier.\n");
  printf("=============================\n\n");

  return 0;
}

/******************************************************************************
* BRACE-ADDITION FUNCTION
******************************************************************************/

// detect & register "smrtassert" & "no C11"/"no compile" macro flags
bool at_smrtassert_or_compile_or_cola_macro_flag(char *p) {
  bool found_macro = false;
  if(is_at_substring(p, "DECLASS_NDEBUG") && !VARCHAR(*(p+strlen("DECLASS_NDEBUG")))) 
    NO_SMRTASSERT = true, found_macro = true;
  else if(is_at_substring(p, "DECLASS_NC11") && !VARCHAR(*(p+strlen("DECLASS_NC11")))) 
    NO_C11_COMPILE_FLAG = true, found_macro = true;
  else if(is_at_substring(p, "DECLASS_NCOMPILE") && !VARCHAR(*(p+strlen("DECLASS_NCOMPILE")))) 
    NO_COMPILE = true, found_macro = true;
  else if(is_at_substring(p, "DECLASS_NCOLA") && !VARCHAR(*(p+strlen("DECLASS_NCOLA"))))
    NO_COLA_PARSER = true, found_macro = true;
  return found_macro;
}

// add braces around any "braceless" single-line conditionals & while/for loops 
// and detects any "#define"'d flags by the user to guide this interpreter's course
void add_braces(char file_contents[]) {
  char BRACED_FILE[MAX_FILESIZE];
  int k, i = 0, j = 0, in_brace_args = 0;
  FLOOD_ZEROS(BRACED_FILE, MAX_FILESIZE);
  BRACED_FILE[j++] = file_contents[i++]; // so "file_contents[i-1]" won't throw error
  bool in_a_string = false, in_a_char = false, in_token_scope = true;

  while(file_contents[i] != '\0') {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, &file_contents[i]);
    // check whether user defined any of the flags in the "DEFN" struct, (STRICT_MODE supersedes all)
    if(in_token_scope && file_contents[i] == '\n' && !(*STRICT_MODE) && !(*NO_DECLASS)) {
      int l = i;
      while(IS_WHITESPACE(file_contents[l])) ++l;
      if(file_contents[l] == '#') {
        ++l;
        while(IS_WHITESPACE(file_contents[l])) ++l;
        if(is_at_substring(&file_contents[l], "define")) {
          l += strlen("define");
          while(IS_WHITESPACE(file_contents[l])) ++l;
          int flag = 0;
          if(is_at_substring(&file_contents[l], "DECLASS_ALLOC_FCNS")) {
            register_user_defined_alloc_fcns(&file_contents[l]);
            // skip over user-defined fcns & don't copy to new file (don't want to #define fcn names redefined later)
            while(file_contents[l] != '\0' && (file_contents[l] != '\n' || file_contents[l-1] == '\\')) ++l;
          } 
          if(at_smrtassert_or_compile_or_cola_macro_flag(&file_contents[l]))
            while(file_contents[l] != '\0' && (file_contents[l] != '\n' || file_contents[l-1] == '\\')) ++l;
          for(; flag < TOTAL_FLAGS; ++flag)
            if(is_at_substring(&file_contents[l], DEFNS.flags[flag]) 
              && !VARCHAR(file_contents[l+strlen(DEFNS.flags[flag])])) {               
              DEFNS.defaults[flag] = DEFNS.non_dflt[flag], i = l + strlen(DEFNS.flags[flag]);
              sprintf(&BRACED_FILE[j], "\n#define %s", DEFNS.flags[flag]);
              j = strlen(BRACED_FILE);
              break;
            }
          if(flag < TOTAL_FLAGS) continue; // check for consecutive flags
        }
      }
    }

    // confirm "#define DECLASS_IGNORE" not found
    if(*NO_DECLASS) throw_DECLASS_IGNORE_message_and_terminate();
    if(*STRICT_MODE) {
      *SMRT_PTRS = *IMMORTALITY = false;
      *DTOR_RETURN = true;
    }

    // check for else if, if, else, while, & for
    for(k = 0; in_token_scope && k < TOTAL_BRACE_KEYWORDS; ++k)
      if(is_at_substring(&file_contents[i],brace_keywords[k]) && !VARCHAR(file_contents[i-1]) && file_contents[i-1] != '#' 
        && !VARCHAR(file_contents[i+strlen(brace_keywords[k])])) {                    // at a "brace keyword"
        for(int l = 0, len = strlen(brace_keywords[k]); l < len; ++l)                 // skip brace keyword
          BRACED_FILE[j++] = file_contents[i++]; 
        while(IS_WHITESPACE(file_contents[i]))                      // skip optional space btwn keyword & '('
          BRACED_FILE[j++] = file_contents[i++];
        if(file_contents[i] == '(' || strcmp(brace_keywords[k], "else") == 0) {       // actual brace keyword
          if(file_contents[i] == '(') {                                               // not else
            BRACED_FILE[j++] = file_contents[i++];                                    // move past '('
            in_brace_args = 1;
            while(file_contents[i] != '\0' && in_brace_args > 0) {                    // copy brace keywords args
              account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, &file_contents[i]);
              if(file_contents[i] == '(')      ++in_brace_args;
              else if(file_contents[i] == ')') --in_brace_args;
              BRACED_FILE[j++] = file_contents[i++]; 
            }
          }
          while(file_contents[i] != '\0' && IS_WHITESPACE(file_contents[i])) { // skip to 1st st8ment after brace keyword
            account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, &file_contents[i]);
            BRACED_FILE[j++] = file_contents[i++]; 
          }
          if(file_contents[i] == ';' || file_contents[i] == '{'                // braced keyword or do-while loop
          ||(file_contents[i]=='d'&&file_contents[i+1]=='o'&&!VARCHAR(file_contents[i+2]))) break;
          BRACED_FILE[j++] = '{';                                              // add brace
          while(file_contents[i] != '\0' && file_contents[i-1] != ';') {       // copy single-line conditional
            account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, &file_contents[i]);
            BRACED_FILE[j++] = file_contents[i++]; 
          }
          BRACED_FILE[j++] = '}';                                              // add brace
        }
        break;                                                                 // found brace keyword, don't check others
      }
    BRACED_FILE[j++] = file_contents[i++];
  }
  BRACED_FILE[j] = '\0';
  FLOOD_ZEROS(file_contents, MAX_FILESIZE);
  strcpy(file_contents, BRACED_FILE); // use "braced" file variant as official file
}

/******************************************************************************
* "DECLASS_H_" HEADER FILE LINKING FUNCTIONS
******************************************************************************/

// parses the file for any "DECLASS_H_" header files & splices them in to the front of the "file_buffer"
void scrape_and_include_all_DECLASS_H_headers(char *file_buffer) {
  int original_declass_header_file_count = 0, i;
  char header_file_contents[MAX_FILESIZE];
  while(true) { // in case "DECLASS_H_" header files included their own other "DECLASS_H_" header files
    original_declass_header_file_count = TOTAL_DECLASS_HEADER_FILES;
    parse_file_for_DECLASS_H_header_files(file_buffer);
    // if new "DECLASS_H_" header files not found
    if(TOTAL_DECLASS_HEADER_FILES <= original_declass_header_file_count) break;
    for(i = original_declass_header_file_count; i < TOTAL_DECLASS_HEADER_FILES; ++i)
      if(is_valid_DECLASS_H_header_file(DECLASS_HEADER_FILE_NAMES[i])) {
        // if valid file to scrape & prepend to current file buffer, 
        // scrape header file contents & prepend to current file buffer
        FLOOD_ZEROS(header_file_contents, MAX_FILESIZE);
        FSCRAPE(header_file_contents, DECLASS_HEADER_FILE_NAMES[i]);
        sprintf(&header_file_contents[strlen(header_file_contents)], "%s\n", file_buffer);
        FLOOD_ZEROS(file_buffer, MAX_FILESIZE);
        strcpy(file_buffer, header_file_contents);
        // remove all new potential comments, trime redundant whitespaces, & check for the reserved "DC_" prefix
        whitespace_all_comments(file_buffer);
        trim_sequential_spaces(file_buffer);
        confirm_no_reserved_DC_prefix(file_buffer);
      }
  }
}

// confirms "DECLASS_H_" header file exists & returns (if "quit" not chosen by user)
// whether to include the header file's contents (if valid) or not (if user chose "continue")
bool is_valid_DECLASS_H_header_file(char *filename) {
  struct stat buf;
  bool parse_header_file = true;
  if(stat(filename, &buf)) {
    fprintf(stderr, "\033[1mdeclass.c:%03d: \033[33mWARNING:\033[0m\033[1m DECLASS HEADER FILE \"%s\" WAS NOT FOUND!\033[0m\n", __LINE__, filename);
    ask_user_whether_to_quit_or_continue_declassification();
    parse_header_file = false;
  } else {
    CUMULATIVE_FILESIZE += (buf.st_size + 1); // +1 for '\n' btwn files
    if(CUMULATIVE_FILESIZE > MAX_FILESIZE || buf.st_size == 0) {
      if(CUMULATIVE_FILESIZE > MAX_FILESIZE) {
        fprintf(stderr, "\033[1mdeclass.c:%03d: \033[33mWARNING:\033[0m\033[1m CUMULATIVE FILESIZE %lld EXCEEDED!\033[0m\n", __LINE__, CUMULATIVE_FILESIZE);
        fprintf(stderr, " >> DECLASS HEADER FILE \"%s\" SIZE %lld BYTES, IN ADDITION TO THE MAIN PROGRAM &\n", filename, buf.st_size);
        fprintf(stderr, "    ITS SPLICED-IN \"declass_\" HEADER FILES, EXCEEDED THE %d BYTE CAP!\n", MAX_FILESIZE); 
        fprintf(stderr, " >> RAISE 'MAX_FILESIZE' MACRO LIMIT!\n");
      } else fprintf(stderr, "\033[1mdeclass.c:%03d: \033[33mWARNING:\033[0m\033[1m CAN'T DECLASSIFY AN EMPTY FILE!\033[0m\n", __LINE__); 
      ask_user_whether_to_quit_or_continue_declassification();
      parse_header_file = false;
    }
  }
  return parse_header_file;
}

// confirms declass header not already stored (prevents duplicate inclusions/splicing in)
bool is_unique_DECLASS_H_header(char *scraped_declass_header_name) {
  for(int i = 0; i < TOTAL_DECLASS_HEADER_FILES; ++i)
    if(strcmp(DECLASS_HEADER_FILE_NAMES[i], scraped_declass_header_name) == 0)
      return false;
  return true;
}

// parses file to scrape any "DECLASS_H_" prefixed header files 
// & whitespaces them out of the buffer
void parse_file_for_DECLASS_H_header_files(char *file_buffer) {
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  char *p = file_buffer, *q, *scout, *include_start, scraped_declass_header_name[MAX_TOKEN_NAME_LENGTH + 8];
  char bad_code_buffer[BAD_CODE_BUFFER_LENGTH];
  int include_lowercased_declass_header_file = 0;
  while(*p != '\0') {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, p);
    if(in_token_scope && is_at_substring(p, "#include") && !VARCHAR(*(p + 8))) { // at a potential "DECLASS_H_" header file
      include_start = p;
      scout = p - (p != file_buffer);               // confirm at a valid "#include"
      // skip optional whitespace prior "#include"
      while(IS_WHITESPACE(*scout) && *scout != '\n' && scout != file_buffer) --scout; 
      // if invalid #include
      if(scout != file_buffer && *scout != '\n') { 
        ++p; 
        continue;
      }
      p += 8;
      // skip optional whitespace after "#include"
      while(*p != '\0' && IS_WHITESPACE(*p)) ++p; 
      ++p; // skip '"' or '<' (a bit presumptuous someone might have a "DECLASS_H_" header in their sys files but hey its called future proofing)
      // if not a declass header
      if(!is_at_substring(p, "DECLASS_H_") && !is_at_substring(p, "declass_h_")) continue;
      scout = p; // check for ".h" extension
      while(VARCHAR(*scout)) ++scout;
      // if at a valid declass header file!
      if(is_at_substring(scout, ".h")) { 
        FLOOD_ZEROS(scraped_declass_header_name, MAX_TOKEN_NAME_LENGTH + 8);
        q = scraped_declass_header_name;
        // copy declass header file name
        while(*p != '\0' && no_overlap(*p, "\">")) *q++ = *p++; 
        *q = '\0', ++p;
        // white-out "DECLASS_H_" header's inclusion
        while(*include_start != '\0' && include_start != p)
          *include_start++ = ' '; 
        // add "DECLASS_H_" header filename if not already found earlier (prevents double inclusion)
        if(is_unique_DECLASS_H_header(scraped_declass_header_name)) {
          strcpy(DECLASS_HEADER_FILE_NAMES[TOTAL_DECLASS_HEADER_FILES], scraped_declass_header_name);
          ++TOTAL_DECLASS_HEADER_FILES;
        }
      }
    }
    ++p;
  }
}

/******************************************************************************
* MESSAGE FUNCTIONS
******************************************************************************/

// confirm (& adjust the appropriate flags) for possible "-l", "-no-compile",
// "-mortal-errors", &/or "-save-temps" cmd-line flags
// (throws error if any other invalid flag found)
void process_cmd_flag(char *flag, bool *show_class_info, bool *save_temp_files) {
  if(strcmp(flag, "-l") == 0)
    *show_class_info = true;
  else if(strcmp(flag, "-no-compile") == 0)
    NO_COMPILE = true;
  else if(strcmp(flag, "-save-temps") == 0)
    *save_temp_files = true;
  else if(strcmp(flag, "-mortal-errors") == 0)
    MORTAL_ERRORS = true;
  else 
    declass_missing_Cfile_alert(); // Invalid cmd-line flag: != "-l" nor "-no-compile" nor "-save-temps"
}

// uncomments smrtptr.h's alerts, invoked if detected "#define DECLASS_NOISYSMRTPTR"
void enable_smrtptr_alerts() {
  char *p = DC_SMART_POINTER_H_;
  while(*p != '\0' && !is_at_substring(p, 
    "// if(SMRTPTR_GC.len > 0) printf(\"FREED %ld SMART POINTERS!\\n\"")) ++p;
  *p++ = ' '; *p++ = ' '; // uncomment smrtptr.h's garbage-collector alert
  while(*p != '\0' && !is_at_substring(p, "// printf(\"SMART POINTER #%ld STORED!\\n\"")) ++p;
  *p++ = ' '; *p++ = ' '; // uncomment smrt-m/calloc alert
  while(*p != '\0' && !is_at_substring(p, "// printf(\"SMART POINTER REALLOC'D!\\n\");")) ++p;
  *p++ = ' '; *p++ = ' '; // uncomment smrtrealloc alert
  while(*p != '\0' && !is_at_substring(p, "// printf(\"SMART POINTER FREED!\\n\");")) ++p;
  *p++ = ' '; *p++ = ' '; // uncomment smrtfree alert
}

// parses file to confirm reserved "DC_" prefix was not used to name any token
void confirm_no_reserved_DC_prefix(char *file_buffer) {
  bool in_a_string = false, in_a_char = false, in_token_scope = true, found_reserved_prefix = false;
  char *p = file_buffer, bad_code_buffer[BAD_CODE_BUFFER_LENGTH];
  if(is_at_substring(p, "DC_")) found_reserved_prefix = true;
  else {
    ++p;
    while(*p != '\0') {
      account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, p);
      if(in_token_scope && !VARCHAR(*(p - 1)) && is_at_substring(p, "DC_")) {
        found_reserved_prefix = true;
        break;
      }
      ++p;
    }
  }
  if(found_reserved_prefix) {
    fprintf(stderr, "\033[1mdeclass.c:%03d: \033[31mERROR\033[0m\033[1m DETECTED RESERVED \"DC_\" PREFIX IN FILE!\033[0m\n >> FOUND HERE:", __LINE__);
    FLOOD_ZEROS(bad_code_buffer, BAD_CODE_BUFFER_LENGTH);
    get_invalid_code_snippet(file_buffer, p, bad_code_buffer);
    fprintf(stderr, "\n\033[1m<CODE_SNIPPET>\n\033[7m%s\033[0m\n\033[1m</CODE_SNIPPET>\033[0m\n\n", bad_code_buffer);
    fprintf(stderr, " >> REMOVE THE RESERVED PREFIX FROM FILE (AS PER CAVEAT #0 SO WAY TO GO READING THE DOCUMENTATION) TO DECLASSIFY FILE!\n");
    if(MORTAL_ERRORS) ask_user_whether_to_quit_or_continue_declassification();
    else {
      fprintf(stderr, " >> Terminating Declassifier.\n");
      exit(EXIT_FAILURE);
    }
  }
}

// confirms file exists, non-empty, & takes less memory than MAX_FILESIZE
void confirm_valid_file(char *filename) {
  struct stat buf;
  if(stat(filename, &buf)) {
    declass_ERROR_ascii_art();
    fprintf(stderr, "\033[1m >> FILE \"%s\" DOES NOT EXIST!\033[0m\n", filename);
    fprintf(stderr, ">> Terminating Declassifier.\n\n");
    exit(EXIT_FAILURE);
  }
  CUMULATIVE_FILESIZE += buf.st_size;
  if(buf.st_size > MAX_FILESIZE || buf.st_size == 0) {
    declass_ERROR_ascii_art();
    if(buf.st_size > MAX_FILESIZE) {
      fprintf(stderr, "\033[1m >> FILE \"%s\" SIZE %lld BYTES EXCEEDS %d BYTE CAP!\033[0m\n", filename, buf.st_size, MAX_FILESIZE); 
      fprintf(stderr, " >> RAISE 'MAX_FILESIZE' MACRO LIMIT!\n");
    } else fprintf(stderr, "\033[1m >> CAN'T DECLASSIFY AN EMPTY FILE!\033[0m\n"); 
    fprintf(stderr, " >> Terminating Declassifier.\n\n");
    exit(EXIT_FAILURE);
  }
}

// confirms command processor exists to compile declassified code autonomously via "system()":
// if DNE declass.c only converts the file w/o compiling, as if "#define DECLASS_NCOMPILE" were found
void confirm_command_processor_exists_for_autonomous_compilation(int line_No) {
  if(!NO_COMPILE && !system(NULL)) { // no need to show if client already disabled auto-compilation
    fprintf(stderr, "\033[1mdeclass.c:%03d: \033[33mWARNING\033[0m\033[1m, Command Processor Does Not Exist!\033[0m\n", line_No);
    fprintf(stderr, " >> Declassifying File Without Compiling.\n >> Client Must Manually Compile Converted Code.\n");
    NO_COMPILE = true;
  }
}

// 'declassified' in spaces with the background & "text" colors reversed
void declass_DECLASSIFIED_ascii_art() {
  const char *rev  = "\033[7m", *not  = "\033[0m";
  printf("\n=================================================================================\n");
  printf("%s   %s    %s    %s   %s    %s  %s  %s     %s   %s     %s    %s   %s    %s  %s   %s  %s     %s %s   %s  %s    %s  %s   %s   %s     %s\n", 
    rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not);
  printf("%s %s %s  %s  %s  %s  %s  %s %s  %s  %s  %s  %s %s     %s %s %s %s    %s  %s  %s",
    rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev);
  printf("  %s %s  %s  %s  %s  %s %s   %s %s   %s %s  %s %s  %s  %s  %s  %s %s %s %s  %s   %s   %s \n", 
    not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not);
  printf("%s %s  %s  %s %s   %s    %s  %s      %s %s     %s   %s     %s  %s     %s  %s    ",
    rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not);
  printf(" %s %s   %s   %s    %s %s  %s   %s    %s %s  %s  %s   %s %s  \n", 
    rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not);
  printf("%s %s %s  %s  %s  %s  %s  %s %s  %s  %s  %s  %s %s %s  %s  %s %s %s %s  %s  %s  %s  %s %s  ",
    rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev);
  printf("%s  %s  %s    %s %s   %s %s      %s %s  %s  %s  %s  %s %s %s %s  %s       \n", 
    not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not);
  printf("%s   %s    %s    %s   %s    %s   %s   %s  %s  %s %s  %s  %s    %s   %s    %s    %s   %s %s  %s     %s   %s  %s    %s  %s   %s    %s   %s ", 
    rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not, rev, not);
  printf("\n=================================================================================\n");
}

// 'error' in ascii
void declass_ERROR_ascii_art() {
  printf("\n========================================\n\033[1m\033[31m");
  printf("  /|===\\ ||^\\\\ ||^\\\\ //==\\\\ ||^\\\\   //\n");
  printf("  ||==   ||_// ||_// ||  || ||_//  //\n");
  printf("  \\|===/ || \\\\ || \\\\ \\\\==// || \\\\ <*>");
  printf("\033[0m\n========================================\n");
}

// error & how-to-execute message
void declass_missing_Cfile_alert() {
  declass_ERROR_ascii_art();
  printf("** Missing .c File Cmd Line Argument! **\n");
  printf("Exe: $ gcc -std=c99 -o declass declass.c\n     $ ./declass yourFile.c");
  printf("\n========================================");
  printf("\n*** Or Else Misused A Cmd Line Flag! ***\n");
  printf("* Can Be Combined! (end w/ \"yourFile.c\")\n");
  printf("Info: $ ./declass -l yourFile.c\n");
  printf("File: $ ./declass -save-temps yourFile.c\n");
  printf("!GCC: $ ./declass -no-compile yourFile.c");
  printf("\n========================================");
  printf("\n********* Filename Conversion: *********\n"); 
  printf("    yourFile.c => yourFile_DECLASS.c    ");
  printf("\n========================================\n");
  printf(" >> Terminating Declassifier.");
  printf("\n=============================\n\n");
  exit(EXIT_FAILURE);
}

// thrown if "#define DECLASS_IGNORE" was detected, terminates program
void throw_DECLASS_IGNORE_message_and_terminate() {
  fprintf(stderr, "\n >> \033[1mdeclass.c: \033[33mWARNING\033[0m\033[1m \"#define DECLASS_IGNORE\" Was Detected!\033[0m");
  fprintf(stderr, "\n\033[1m >> Terminating Declassifier.\033[0m\n\n");
  exit(EXIT_FAILURE);
}

// output class data if argv[1] == '-l' flag
void show_l_flag_data() {
  if(total_classes > 0) printf("\n\033[1m--=[ TOTAL CLASSES: %d ]=--", total_classes);
  (total_objects > 0) ? printf("=[ TOTAL OBJECTS: %d ]=--\033[0m\n", total_objects) : printf("\033[0m\n");
  for(int i = 0; i < total_classes; ++i) {
    int class_objects_sum = 0;
    for(int j = 0; j < total_objects; ++j) 
      if(strcmp(classes[i].class_name, objects[j].class_name) == 0) class_objects_sum++;
    printf("\n\033[1mCLASS No%d, %s:\033[0m\n", i + 1, classes[i].class_name);

    int total_members = classes[i].total_members; // differentiate between class struct member members & class members
    for(int j = 0; j < classes[i].total_members; ++j) if(classes[i].member_names[j][0] == 0) --total_members;
    if(total_members > 0) {
      printf(" L_ \033[1mMEMBERS: %d\033[0m\n", total_members);
      for(int j = 0; j < classes[i].total_members; ++j) {
        if(classes[i].member_names[j][0] == 0) continue;
        char bar = (classes[i].total_methods > 0) ? '|' : ' ';
        if(classes[i].member_is_pointer[j])    printf(" %c  L_ *%s", bar, classes[i].member_names[j]);
        else if(classes[i].member_is_array[j]) printf(" %c  L_ %s[]", bar, classes[i].member_names[j]);
        else printf(" %c  L_ %s", bar, classes[i].member_names[j]);
        if(classes[i].member_value_is_alloc[j]) printf(" (( ALLOCATED MEMORY ))");
        if(classes[i].member_is_immortal[j]) printf(" (( IMMORTAL ))");
        printf("\n");
      }
    }

    char method_name[MAX_TOKEN_NAME_LENGTH];
    if(classes[i].total_methods > 0) {
        printf(" L_ \033[1mMETHODS: %d\033[0m\n", classes[i].total_methods);
      for(int j = 0; j < classes[i].total_methods; ++j) {
        FLOOD_ZEROS(method_name, MAX_TOKEN_NAME_LENGTH);
        if(strcmp(classes[i].method_names[j], "DC__constructor") == 0)
          sprintf(method_name, "%s() (( CONSTRUCTOR ))", classes[i].class_name);
        else if(strcmp(classes[i].method_names[j], "DC__destructor") == 0)
          sprintf(method_name, "~%s() (( DESTRUCTOR ))", classes[i].class_name);
        else sprintf(method_name, "%s()", classes[i].method_names[j]);
        (class_objects_sum > 0) ? printf(" | L_ %s\n", method_name) : printf("   L_ %s\n", method_name);
      }
    }

    if(class_objects_sum > 0) {
      printf(" L_ \033[1mOBJECTS: %d\033[0m\n", class_objects_sum);
      for(int j = 0; j < total_objects; ++j) 
        if(strcmp(classes[i].class_name, objects[j].class_name) == 0) {
          if(objects[j].is_class_pointer)    printf("   L_ *%s", objects[j].object_name);
          else if(objects[j].is_class_array) printf("   L_ %s[]", objects[j].object_name);
          else printf("   L_ %s", objects[j].object_name);
          if(objects[j].is_immortal) printf(" (( IMMORTAL ))");
          printf("\n");
        }
    }
  }
}

// confirms w/ user as to whether declass.c ought to terminate it's parsing (generally
// recommended) or to continue attempting declassification
void ask_user_whether_to_quit_or_continue_declassification() {
  printf(" >> CONTINUE DECLASSIFICATION PROCESS? ENTER 1 FOR YES & 0 FOR NO\n\n>>> ");
  int continueDeclassification;
  scanf("%d", &continueDeclassification);
  if(continueDeclassification == 1)
    printf("\n\n-:- CONTINUING DECLASSIFICATION PROCESS - \033[1mI HOPE YOU KNOW WHAT YOU'RE DOING\033[0m -:-\n\n");
  else {
    printf("\n\n >> Terminating Declassifier.\n\n");
    exit(EXIT_SUCCESS); // having been intentionally terminated
  }
}

// triggered when a potential dummy ctor was found as an arg in a method
// (highly unlikely but still a corner case) either continues declassifying
// or terminates declass.c's conversion as per user's input
void POSSIBLE_DUMMY_CTOR_METHOD_ARG_ERROR_MESSAGE(const char fcn[12],int line,int class_idx) {
  fprintf(stderr, 
    "\033[1mdeclass.c: \033[33mWARNING\033[0m\033[1m UNDEFINED BEHAVIOR IN FCN: \"%s\", LINE: %d\033[0m\n", fcn, line);
  fprintf(stderr, 
    " >> DETECTED \"%s(\" POTENTIAL \"DUMMY\" CTOR INVOCATION IN ARGS OF METHOD: \"%s\" IN CLASS: \"%s\"\n", 
    classes[class_idx].class_name, classes[total_classes].method_names[classes[total_classes].total_methods-1], 
    classes[class_idx].class_name);
  fprintf(stderr, " >> DUMMY CTORS CAN RETURN OBJECTS, BUT NEVER REPRESENT THEM AS A VARIABLE!\n");
  ask_user_whether_to_quit_or_continue_declassification();
}

// thrown if during "object pointer being constructed and alloc'd" parsing a double assignment 
// value not prefixed w/ "DC_" was detected (thus not a prefixed ctor as had been expected) 
// (such being only instance where a valid double assignment could occur, and even then it's 
// generated by declass.c not the user)
void throw_potential_invalid_double_dflt_assignment(const char fcn[12], int line, char *class_name) {
  fprintf(stderr, "\033[1mdeclass.c: \033[33mWARNING\033[0m\033[1m UNDEFINED BEHAVIOR IN FCN: \"%s\", LINE: %d\033[0m\n", fcn, line);
  fprintf(stderr, 
    " >> DETECTED POTENTIAL DOUBLE DEFAULT-VALUE ASSINGMENT IN CLASS \"%s\" MEMBER DECLARATIONS\n", class_name);
  fprintf(stderr, " >> NORMALLY OCCURS WHEN ALLOCATING & CONSTRUCTING A POINTER MEMBER AT ONCE\n");
  fprintf(stderr, " >> IE: \"ClassName *objectName(args) = smrtmalloc(sizeof(className));\"\n");
  ask_user_whether_to_quit_or_continue_declassification();
}

// thrown if a user-defined an invalid DTOR that took args was detected
void throw_fatal_error_dtor_has_args(char *class_name, int line_No, char *bad_code_buffer) {
  fprintf(stderr, "\033[1mdeclass.c:%03d: \033[31mERROR\033[0m\033[1m FOUND INVALID DTOR THAT ACCEPTS ARGS IN CLASS: \"%s\"!\033[0m\n >> FOUND HERE:\n", 
    line_No, class_name);
  fprintf(stderr, "\n\033[1m<CODE_SNIPPET>\n\033[7m%s\033[0m\n\033[1m</CODE_SNIPPET>\033[0m\n\n", bad_code_buffer);
  fprintf(stderr, " >> RMV ARGS FROM DTOR IN ORDER TO BE VALID FOR DECLASSIFICATION! DTORS CAN \033[1mNEVER\033[0m TAKE ARGS!\n");
  if(MORTAL_ERRORS) ask_user_whether_to_quit_or_continue_declassification();
  else {
    fprintf(stderr, " >> Terminating Declassifier.\n");
    exit(EXIT_FAILURE);
  }
}

/******************************************************************************
* USER-DEFINED ALLOCATION FUNCTIONS PARSING/REGISTERING FUNCTIONS
******************************************************************************/

// returns whether at a substring with either an stdlib.h, smrtptr.h, or user-defined allocation fcn
bool is_an_alloc_fcn(char *str) {
  for(int i = 0; i < TOTAL_ALLOC_FCNS; ++i) 
    if(is_at_substring(str, ALLOC_FCNS[i]) && !VARCHAR(*(str + strlen(ALLOC_FCNS[i]))))
      return true;
  return false;
}

// given a line prefixed with "#define DECLASS_ALLOC_FCNS" parses out any user-defined
// allocation fcns subsequently following
void register_user_defined_alloc_fcns(char *s) { 
  // alloc fcns must be seperate & on the same line, aside from that use spaces, commas,
  // etc. (whatever floats your boat aesthetically)
  int row = 0;
  char *p = s;
  while(!is_at_substring(p, "DECLASS_ALLOC_FCNS")) ++p; // skip past "#define" flag
  p += strlen("DECLASS_ALLOC_FCNS");
  while(*p != '\0' && (*p != '\n' || *(p-1) == '\\')) { // while still more user-defined alloc fcns
    if(VARCHAR(*p)) { // copy the current user-defined alloc fcn
      row = 0;
      while(VARCHAR(*p)) ALLOC_FCNS[TOTAL_ALLOC_FCNS][row++] = *p++;
      ALLOC_FCNS[TOTAL_ALLOC_FCNS++][row] = '\0';
    }
    if(*p != '\n' || *(p-1) == '\\') ++p; // skip past non alloc fcns
  }
}

/******************************************************************************
* COMMENT & BLANK LINE SKIPPING FUNCTIONS
******************************************************************************/

// replaces all comments inside of a class instance with spaces
void whitespace_all_comments(char *end) { // end = 1 past 1st '{'
  int in_class_scope = 1;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  while(*end != '\0' && in_class_scope > 0) {
    // confirm in class' scope
    if(*end == '{') in_class_scope++;
    else if(*end == '}') in_class_scope--;
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, end);
    if(in_class_scope < 0) break;
    if(in_token_scope && *end == '/' && *(end + 1) == '/') {
      while(*end != '\0' && *end != '\n') *end++ = ' ';
    } else if(in_token_scope && *end == '/' && *(end + 1) == '*') {
      *end++ = ' ', *end++ = ' ';
      while(*end != '\0' && (*end != '*' || *(end + 1) != '/')) *end++ = ' ';
      *end++ = ' ', *end++ = ' ';
    }
    end++;
  }
}

// trims any sequences of spaces ended by ('\n' || ';' || '=') to just ('\n' || ';' || '=') 
// in "OLD_BUFFER" -- also trims any sequence of '\n' down to a max of 3 '\n'
void trim_sequential_spaces(char OLD_BUFFER[]) {
  char *read = OLD_BUFFER, NEW_BUFFER[MAX_FILESIZE], *scout, *write;
  FLOOD_ZEROS(NEW_BUFFER, MAX_FILESIZE);
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  write = NEW_BUFFER;
  *write++ = *read++;                                   // so first string check doesn't check garbage memory
  while(*read != '\0') {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, read);
    if(in_token_scope && *read == ' ') {                  // trims space(s) + ('\n' || ';' || '=') sequence
      scout = read;
      while(*scout != '\0' && *scout == ' ') ++scout;   // skip over spaces
      if(!no_overlap(*scout, "\n;")) read = scout;      // if correct format, passover spaces
      else if(*scout == '=') read = scout - 1;
    } else if(in_token_scope && is_at_substring(read, "\n\n\n\n")) { // trim any '\n' sequences length > 3 down to 3
      for(int i = 0; i < 3; ++i) *write++ = *read++;
      scout = read;
      while(*scout != '\0' && *scout == '\n') ++scout;
      read = scout;
    }
    *write++ = *read++;
  }
  *write = '\0';
  FLOOD_ZEROS(OLD_BUFFER, MAX_FILESIZE);
  strcpy(OLD_BUFFER, NEW_BUFFER);
}

// gives size to skip of lines composed solely of whitespaces with '\n's at either end
int remove_blank_lines(char *end) {
  char *blank_char = end + 1;               // 'end' starts at '\n'
  int total_blank_chars = 0;
  while(true) {                             // while still more blank lines
    int blank_line_size = 1;
    while(*blank_char != '\0' && IS_WHITESPACE(*blank_char) && *blank_char != '\n')
      blank_char++, blank_line_size++;      // traverse up to next '\n' or non-whitespace
    if(*blank_char == '\n') {               // if an entire line of only whitespaces
      total_blank_chars += blank_line_size; // increment skipping-size by blank line's length
      blank_char++;                         // move to next line/char
    } else return total_blank_chars;
  }
}

/******************************************************************************
* STRING HELPER FUNCTIONS
******************************************************************************/

// returns whether c exists in '*bad_chars'
bool no_overlap(char c, char *bad_chars) {
  while(*bad_chars != '\0') if(*bad_chars++ == c) return false;
  return true;
}

// returns if 'p' points to the particular substring 'substr'
bool is_at_substring(char *p, char *substr) {
  while(*substr != '\0' && *p != '\0' && *p == *substr) ++p, ++substr;
  return (*substr == '\0');
}

// checks if p is in a string or char, assigning the respective "bool" ptrs accordingly
void account_for_string_char_scopes(bool *in_a_string, bool *in_a_char, bool *in_token_scope, char *p) {
  if(!(*in_a_char)   && *p == '"'  && (*(p-1) != '\\' || *(p-2) == '\\')) *in_a_string = !(*in_a_string);
  if(!(*in_a_string) && *p == '\'' && (*(p-1) != '\\' || *(p-2) == '\\')) *in_a_char   = !(*in_a_char);
  *in_token_scope = (!(*in_a_string) && !(*in_a_char));
}

/******************************************************************************
* DUMMY CTOR OBJECT INVOCATION CHAIN PARSER
******************************************************************************/

// given "read" ptr @ 1st letter of dummy ctor's class name, fills the "invoking CHAIN"
// string, assigns whether or not the invoking NAME is an object ptr, & assigns whether object being redefined
// (thus not declared).
void get_dummy_ctor_invoking_object_chain(char *read, char *invoking_chain, int class_idx, bool *invoked_by_ptr, bool *is_redefined) {
  // suppose object o1 (of class c1) contains object o2 (of class c2). 
  // Initializing "o2" w/ its class' dummy ctor "c2()": "o1.o2 = c2();"
  // INVOKING CHAIN: "o1.o2" -- INVOKING NAME:  "o2"
  bool in_a_string = false, in_a_char = false, in_token_scope = *is_redefined = true; // *is_redefined becomes false if declaration found
  int in_array_scope = 0;
  char *p = read - 1; // 1 position directly behind dummy ctor invocation class name
  char *q = invoking_chain, *invocation_end, *scout;
  char invoking_name[MAX_TOKEN_NAME_LENGTH * 2], *last_member_invocation_punctuation = NULL;
  FLOOD_ZEROS(invoking_name, MAX_TOKEN_NAME_LENGTH * 2); // * 2 in case = an object array cell accessed via an array subscript 
  // move to the beginning of the invoking object/chain
  while(*p != '=') --p;         // move to '='
  --p;                          // move 1 behind '='
  while(IS_WHITESPACE(*p)) --p; // move to 1st position in invocation chain
  invocation_end = p + 1;       // 1 past the end
  while(!in_token_scope || VARCHAR(*p) || !no_overlap(*p, " \t.]*&") || (*p=='>'&&*(p-1)=='-')) {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, p);
    if(last_member_invocation_punctuation != NULL && (*p == '.' || *p == '>')) // save the 1st invocation punct encountered
      last_member_invocation_punctuation = p;
    if(in_token_scope && *p == ']') { // skip array subscript contents
      in_array_scope = 1, --p;
      while(in_array_scope > 1) {     // ends w/ "p" 1 position behind the opening '['
        account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, p);
        if(in_token_scope && *p == ']')      ++in_array_scope;
        else if(in_token_scope && *p == '[') --in_array_scope;
        --p;
      }
    } else if(*p == ' ' || *p == '\t') {
      // at a non-'\n' whitespace, confirm not preceded by a letter or '_' ,
      // as this indicates possible declaration v.s. spacing btwn puncts, ie '[]', '->', etc.
      scout = p - 1;
      while(IS_WHITESPACE(*scout)) --scout;
      if(VARCHAR(*scout)) {             // check if a declaration (ie NOT a redefinition)
        while(VARCHAR(*scout)) --scout; // move to the just prior the preceding word (or class name if declaration)
        ++scout;                        // move to first letter of the preceding word 
        *is_redefined = !is_at_substring(scout, classes[class_idx].class_name);
        break;
      }
    }
    --p;
  }
  // copy the invoking object/chain
  while(IS_WHITESPACE(*p)) ++p;           // skip to the front of the invoker's name
  while(p != invocation_end) *q++ = *p++; // copy invoker name
  *q = '\0';
  // determine if invoked by ptr object (get invoker name & search for ptr member status in "objects" struct array)
  if(last_member_invocation_punctuation != NULL) {
    q = invoking_name;
    ++last_member_invocation_punctuation; // move 1 position past the 1st (going backwards) invocation punct found
    while(last_member_invocation_punctuation != invocation_end)
      *q++ = *last_member_invocation_punctuation++; // copy invoking object's name (NOT the entire chain)
    *q = '\0';
  } else strcpy(invoking_name, invoking_chain);

  for(int i = 0; i < total_objects; ++i)
    if(strcmp(objects[i].object_name, invoking_name) == 0 && strcmp(objects[i].class_name, classes[class_idx].class_name) == 0) {
      *invoked_by_ptr = objects[i].is_class_pointer;
      return;
    }
}

/******************************************************************************
* OBJECT CONSTRUCTION FUNCTIONS
******************************************************************************/

// returns whether member value is a dummy ctor -- ie a class name
bool is_a_dummy_ctor(char *member_value) {
  for(int i = 0; i < total_classes; ++i)
    if(is_at_substring(member_value, classes[i].class_name) 
      && !VARCHAR(*(member_value + strlen(classes[i].class_name))))
      return true;
  return false;
}

// given a class index, returns an initialization brace for it's member values
void mk_initialization_brace(char brace[], int class_index) {
  char *p = brace;
  strcpy(p, "{&DC__NDTR,");
  p += strlen(p);
  for(int j = 1; j < classes[class_index].total_members; ++j) { // 1 not 0 to skip over dflt included "object has been dtor'd flag" member
    if(classes[class_index].member_values[j][0] == 0) { // empty value
      // if struct's member (struct inner members' name = value = 0), skip
      if(classes[class_index].member_names[j][0] == 0) continue;
      else if(classes[class_index].member_is_array[j] || (j > 1 && classes[class_index].member_names[j-1][0] == 0)
        || (classes[class_index].member_object_class_name[j][0] != 0 && !classes[class_index].member_is_pointer[j]))
          sprintf(p, "{0},"); // wrap empty (non-ptr) array/object/struct value in braces
      else if((j > 1 && classes[class_index].member_names[j-1][0] != 0) || j == 1) 
        sprintf(p, "0,"); 
    } else {
      if(is_a_dummy_ctor(classes[class_index].member_values[j])) // prefix dummy ctor names as needed
        sprintf(p, "DC__DUMMY_"), p += strlen(p); 
      sprintf(p, "%s,", classes[class_index].member_values[j]);  // non-empty value
    }
    p += strlen(p);
  }
  *p++ = '}';
  *p = '\0';
}

// appends ctor-macros for object arrays to the "ctor_macros" string
void mk_object_array_ctor_macros(char ctor_macros[], char *class_name) {
  char *p = ctor_macros;
  if(classes[total_classes].total_uctors == 0) { 
    // add a macro to invoke the dflt ctor for object arrays
    sprintf(p, "\n#define DC__%s_UCTOR_ARR(DC_ARR) ({\\\n\
  for(int DC__%s_UCTOR_IDX=0;DC__%s_UCTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));\
++DC__%s_UCTOR_IDX)\\\nDC_%s_(&DC_ARR[DC__%s_UCTOR_IDX]);\\\n})", class_name, class_name, class_name, class_name, class_name, class_name);
  } else { 

    // add macros to invoke user-defined ctors for object arrays
    for(int i = 0, j, number_of_args; i < classes[total_classes].total_uctors; ++i) {
      number_of_args = classes[total_classes].class_uctor_arg_lengths[i];
      // no-arg user-def'd ctor object array macro
      if(number_of_args == 0) { 
        sprintf(p, "\n#define DC__%s_UCTOR_ARR(DC_ARR) ({\\\n\
  for(int DC__%s_UCTOR_IDX=0;DC__%s_UCTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));\
++DC__%s_UCTOR_IDX)\\\nDC_%s_(&DC_ARR[DC__%s_UCTOR_IDX]);\\\n})", class_name, class_name, class_name, class_name, class_name, class_name);

      // has-arg(s) user-def'd ctor object array macro
      } else {
        sprintf(p, "\n#define DC__%s_UCTOR_ARR(DC_ARR", class_name), p += strlen(p);
        // sprintf args variables
        for(j = 0; j < number_of_args; ++j)
          sprintf(p, ", DC___A%d_%s", j + 1, class_name), p += strlen(p);
        // sprintf loop to iterate over object array's individual objects
        sprintf(p, ") ({\\\n\
  for(int DC__%s_UCTOR_IDX=0;DC__%s_UCTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));\
++DC__%s_UCTOR_IDX)\\\n    DC_%s_(", class_name, class_name, class_name, class_name);
        p += strlen(p);
        // sprintf args in user-def'd ctor invocation per object in array loop-iteration
        for(j = 0; j < number_of_args; ++j)
          sprintf(p, "DC___A%d_%s, ", j + 1, class_name), p += strlen(p);
        // pass object in object array to user-defined ctor
        sprintf(p, "&DC_ARR[DC__%s_UCTOR_IDX]);\\\n})", class_name);
      }
      p += strlen(p);
    }
  }
  p += strlen(p);
  *p = '\0';
}

// fills "ctor_macros" string w/ macros for both single & array object constructions/initializations
void mk_ctor_macros(char ctor_macros[], char *class_name) {
  char *p = ctor_macros;
  // add macro for a single object construction instance
  sprintf(p, "#define DC__%s_CTOR(DC_THIS) ({DC_THIS = DC__%s_DFLT();", class_name, class_name);
  p += strlen(p);
  // search for members that are also other class objects
  for(int l = 0; l < classes[total_classes].total_members; ++l) {
    if(classes[total_classes].member_object_class_name[l][0] != 0) { // member = class object
      // append macros to initialize any members that are class objects
      if(classes[total_classes].member_is_array[l]) // append arr ctor
        sprintf(p, "\\\n\tDC__%s_ARR(DC_THIS.%s);",     classes[total_classes].member_object_class_name[l], classes[total_classes].member_names[l]);
      else if(classes[total_classes].member_value_is_alloc[l]) // append 1 obj ctor
        sprintf(p, "\\\n\tDC__%s_CTOR(*(DC_THIS.%s));", classes[total_classes].member_object_class_name[l], classes[total_classes].member_names[l]);
      else if(!classes[total_classes].member_is_pointer[l])    // append 1 obj ctor
        sprintf(p, "\\\n\tDC__%s_CTOR(DC_THIS.%s);",    classes[total_classes].member_object_class_name[l], classes[total_classes].member_names[l]);
      p += strlen(p);
      // check for any user-defined array ctors (all macros by default) that ought to be invoked
      // to be invoked outside of the brace initialization
      if(classes[total_classes].member_value_user_ctor[l][0] != 0)
        sprintf(p, "\\\n\t%s;", classes[total_classes].member_value_user_ctor[l]), p += strlen(p);
    }
  }
  sprintf(p, "})"), p += strlen(p);

  // add macro for a an array of object constructions
  sprintf(p, "\n#define DC__%s_ARR(DC_ARR) ({\\\n\
  for(int DC__%s_IDX=0;DC__%s_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__%s_IDX)\\\n\
    DC__%s_CTOR(DC_ARR[DC__%s_IDX]);\\\n})", 
    class_name, class_name, class_name, class_name, class_name, class_name);
  p += strlen(p);

  // add macros for arrays of user-defined object construction(s (plural if overloaded))
  mk_object_array_ctor_macros(p, class_name);
}

// make global initializing function to assign default values
void mk_class_global_initializer(char *class_global_initializer, char *class_name, char *initial_values_brace) {
  sprintf(class_global_initializer, "\n%s DC__%s_DFLT(){\n\t%s this=%s;\n\treturn this;\n}", 
    class_name, class_name, class_name, initial_values_brace);
}

// prefixes any user invocations of a "dummy" class constructor w/ "DC__DUMMY_"
int prefix_dummy_ctor_with_DC__DUMMY_(char *write, char *read) {
  int dummy_ctor_len = 0, in_dummy_args_scope = 1;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  sprintf(write, "DC__DUMMY_");
  write += strlen(write);
  while(*read != '(') *write++ = *read++, ++dummy_ctor_len;
  *write++ = *read++, ++dummy_ctor_len; // copy '('
  while(*read != '\0' && in_dummy_args_scope > 0) { // copy dummy ctor args
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, read);
    if(in_token_scope && *read == '(')      ++in_dummy_args_scope; 
    else if(in_token_scope && *read == ')') --in_dummy_args_scope;
    *write++ = *read++, ++dummy_ctor_len;
  }
  return dummy_ctor_len;
}

// append dummy ctor(s) (for either dflt provided or user-def'd) macro(s) to the "default_cd" string
void mk_dummy_ctor_macros(char *default_cd, char *class_name) {
  if(classes[total_classes].total_uctors == 0) { 
    // provide a dummy ctor for the default provided ctor
    sprintf(default_cd, 
    "\n#define DC__DUMMY_%s()({\\\n\t%s DC__%s__temp;\\\n\tDC__%s_CTOR(DC__%s__temp);\\\n\tDC_%s_(&DC__%s__temp);\\\n})", 
      class_name, class_name, class_name, class_name, class_name, class_name, class_name);
    default_cd += strlen(default_cd);
  } else { 

    // provide a dummy ctor for each instance (overloaded or otherwise) of the user's own defined ctor
    for(int i = 0, j, number_of_args; i < classes[total_classes].total_uctors; ++i) {
      number_of_args = classes[total_classes].class_uctor_arg_lengths[i];
      // no-arg dummy ctor
      if(number_of_args == 0) { 
        sprintf(default_cd, 
          "\n#define DC__DUMMY_%s()({\\\n\t%s DC__%s__temp;\\\n\tDC__%s_CTOR(DC__%s__temp);\\\n\tDC_%s_(&DC__%s__temp);\\\n})", 
          class_name, class_name, class_name, class_name, class_name, class_name, class_name);
      
      // has-arg dummy ctor
      } else {
        sprintf(default_cd, "\n#define DC__DUMMY_%s(", class_name);
        default_cd += strlen(default_cd);
        // add "class_uctor_arg_lengths[i]" # of args to dummy uctor macro's arg list
        for(j = 0; j < number_of_args; ++j) {
          sprintf(default_cd, "DC___D%d_%s", j + 1, class_name), default_cd += strlen(default_cd);
          if(j < number_of_args - 1) sprintf(default_cd, ", "), default_cd += strlen(default_cd);
        }
        sprintf(default_cd, ")({\\\n\t%s DC__%s__temp;\\\n\tDC__%s_CTOR(DC__%s__temp);\\\n\tDC_%s_(", 
          class_name, class_name, class_name, class_name, class_name);
        default_cd += strlen(default_cd);
        // add "class_uctor_arg_lengths[i]" # of args to the dflt-val assignment & declass.c-generated CTOR object initializer macro
        for(j = 0; j < number_of_args; ++j)
          sprintf(default_cd, "DC___D%d_%s, ", j + 1, class_name), default_cd += strlen(default_cd);
        sprintf(default_cd, " &DC__%s__temp);\\\n})", class_name);
      }
      default_cd += strlen(default_cd);
    }
  }
  *default_cd = '\0';
}

/******************************************************************************
* USER-DEFINED OBJECT CONSTRUCTOR (CTOR) PARSING FUNCTIONS
******************************************************************************/

// returns whether object declaration is invoking a user-defined constructor
// & fills the "user_ctor" string w/ the invocation if so
// USED FOR DECLARATIONS IN METHODS/FCNS
bool get_user_ctor(char *p, char *user_ctor, char *class_name, bool *is_fcn_returning_obj) {
  while(*p != '\0' && no_overlap(*p, "\n(;,=")) ++p;
  if(*p != '(') return false; // object not being constructed via user-defined ctor
  char ctor_vals[MAX_DEFAULT_VALUE_LENGTH]; FLOOD_ZEROS(ctor_vals, MAX_DEFAULT_VALUE_LENGTH);
  int val_idx = 0, in_ctor_args = 1;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;

  // copy inner ctor args to "ctor_vals" & white-out "p" buffer's "(<args>)" w/ spaces
  char *whiteout_start = p;
  ++p;                                                          // move past first '('
  while(*p != '\0' && in_ctor_args > 0) {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, p);
    if(in_token_scope && *p == '(')      ++in_ctor_args;          // confirm still in ctor arg scope
    else if(in_token_scope && *p == ')') --in_ctor_args;
    if(in_ctor_args == 0) break;                                // if out of scope
    ctor_vals[val_idx++] = *p++;                                // copy ctor args
  }
  ctor_vals[val_idx] = '\0';
  if(*p == ')') ++p;                                            // move past last ')'

  // check whether p is actually pointing to a fcn that returns an obj
  char *q = p;
  while(*q != '\0' && no_overlap(*q, ";{")) ++q;
  if(*q == '{') {
    *is_fcn_returning_obj = true;
    return false;
  }

  // not a fcn returning an obj, thus whiteout user's ctor args that're being spliced out
  while(whiteout_start != p) *whiteout_start++ = ' ';
  // create user-defined ctor invocation
  if(objects[total_objects-1].is_class_array) { // invoke ctor for each object in object array declaration
    if(strlen(ctor_vals) > 0)
      sprintf(user_ctor, "DC__%s_UCTOR_ARR(%s, %s);",class_name,objects[total_objects-1].object_name,ctor_vals);
    else sprintf(user_ctor, "DC__%s_UCTOR_ARR(%s);", class_name, objects[total_objects-1].object_name);
  } else if(objects[total_objects-1].is_class_pointer) { // invoke ctor for single object declaration
    if(strlen(ctor_vals) > 0)
      sprintf(user_ctor, "DC_%s_(%s, %s);", class_name, ctor_vals, objects[total_objects-1].object_name);
    else sprintf(user_ctor, "DC_%s_(%s);", class_name, objects[total_objects-1].object_name);
  } else {                                      // invoke ctor for single object declaration
    if(strlen(ctor_vals) > 0)
      sprintf(user_ctor, "DC_%s_(%s, &%s);", class_name, ctor_vals, objects[total_objects-1].object_name);
    else sprintf(user_ctor, "DC_%s_(&%s);", class_name, objects[total_objects-1].object_name);
  }
  return true;
}

// check for a user-defined constructor invocation
// USED FOR DECLARATIONS IN CLASS PROTOTYPES
char *check_for_ctor_obj(char*end,char*struct_buff_idx,int*class_size,int*struct_inc,bool*found_ctor){
  char *is_ctor = end;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  *found_ctor = false;
  *struct_inc = 0;
  // from initial '(', traverse the rest of the line & confirm ends w/ ';' not '{'
  while(*is_ctor != '\0' && *is_ctor != '\n') { 
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, is_ctor);
    if(in_token_scope && (*is_ctor == ';' || *is_ctor == '{')) break; // either ctor or method
    ++is_ctor;
  }
  
  // obj ctor as default val for an obj member, NOT a method declaration
  if(*is_ctor == ';') { 
    *found_ctor = true;
    char ctored_class[MAX_TOKEN_NAME_LENGTH], ctored_obj[MAX_TOKEN_NAME_LENGTH], appended_ctor_object[MAX_TOKEN_NAME_LENGTH];
    char *ctored_class_ptr = ctored_class, *ctored_obj_ptr = ctored_obj;
    char *ctoring_invokers = end - 1, *check_no_ctor_args = is_ctor;
    FLOOD_ZEROS(ctored_obj, MAX_TOKEN_NAME_LENGTH); FLOOD_ZEROS(ctored_class, MAX_TOKEN_NAME_LENGTH); 
    FLOOD_ZEROS(appended_ctor_object, MAX_TOKEN_NAME_LENGTH);
    while(no_overlap(*ctoring_invokers, "\n;")) --ctoring_invokers;               // move to the end of the line
    while(!VARCHAR(*ctoring_invokers)) ++ctoring_invokers;                        // move to class name "type"
    while(VARCHAR(*ctoring_invokers))  *ctored_class_ptr++ = *ctoring_invokers++; // copy class of object being ctor'd
    
    *ctored_class_ptr = '\0';
    // if at "immortal" prefix: empty "ctored_class" & re-scrape the class name again but AFTER the keyword
    if(*IMMORTALITY && strcmp(ctored_class, "immortal") == 0) { 
      FLOOD_ZEROS(ctored_class, MAX_TOKEN_NAME_LENGTH); 
      ctored_class_ptr = ctored_class;
      while(!VARCHAR(*ctoring_invokers)) ++ctoring_invokers; // skip space between "immortal" & class name
      while(VARCHAR(*ctoring_invokers))                      // copy class name "type" after "immortal" keyword
        *ctored_class_ptr++ = *ctoring_invokers++;
    }

    while(!VARCHAR(*ctoring_invokers)) ++ctoring_invokers;                        // move to object name
    while(VARCHAR(*ctoring_invokers))  *ctored_obj_ptr++ = *ctoring_invokers++;   // copy object name
    *ctored_class_ptr = '\0', *ctored_obj_ptr = '\0';

    // check whether object ctor is for a single or array of objects
    char *check_array = end - 1, ctor_fcn_assignment[MAX_TOKEN_NAME_LENGTH * 2];
    FLOOD_ZEROS(ctor_fcn_assignment, MAX_TOKEN_NAME_LENGTH * 2);
    while(IS_WHITESPACE(*check_array)) --check_array;
    bool array_ctor = (*check_array == ']');
    if(array_ctor) 
      sprintf(ctor_fcn_assignment, " = DC__%s_UCTOR_ARR", ctored_class); 
    else sprintf(ctor_fcn_assignment, " = DC_%s_", ctored_class); 

    // check whether object ctoring is a ptr
    char *check_ptr = check_array;
    if(array_ctor) while(*check_ptr != '[') --check_ptr; // skip past array subscript if present
    while(no_overlap(*check_ptr, "\n*{};")) --check_ptr;
    bool ptr_ctored = (*check_ptr == '*');

    // check whether obj is a pointer being ctor'd & alloc'd at once 
    // (causes "obj = ctor = malloc" and can make "LAST_ARG" point to "malloc" args & 
    // splice "DC_THIS.%s" in the wrong fcn)
    char *allocd_ptr_obj = check_no_ctor_args;
    while(no_overlap(*allocd_ptr_obj, "\n=")) --allocd_ptr_obj; // check if supposed "ctor" is assigned
    if(*allocd_ptr_obj == '=') {
      // check if assigned fcn is preceded by another fcn (ie if "alloc" preceded by some ctor)
      while(no_overlap(*allocd_ptr_obj, "\n)")) --allocd_ptr_obj; 
      if(*allocd_ptr_obj == ')') check_no_ctor_args = allocd_ptr_obj; // actual ctor preceding allocation
    }

    // check whether ctor default value has args
    while(*check_no_ctor_args != ')') --check_no_ctor_args;         // find end of ctor
    --check_no_ctor_args;                                           // move past last ')'
    while(IS_WHITESPACE(*check_no_ctor_args)) --check_no_ctor_args; // if directly preceded by '(' ctor = empty
    if(*check_no_ctor_args == '(') {                                // splice "&DC_THIS.objName" to end of ctor args
      if(array_ctor||ptr_ctored) sprintf(appended_ctor_object, "DC_THIS.%s", ctored_obj);
      else                       sprintf(appended_ctor_object, "&DC_THIS.%s", ctored_obj);
    } else if(array_ctor)        sprintf(appended_ctor_object, "DC_THIS.%s, ", ctored_obj);
    else if(ptr_ctored)          sprintf(appended_ctor_object, ", DC_THIS.%s", ctored_obj);
    else                         sprintf(appended_ctor_object, ", &DC_THIS.%s", ctored_obj);
    char *LAST_ARG = check_no_ctor_args, *FIRST_ARG = end;
    
    // confirm class of object with default ctor value exists
    int is_a_defined_class = 0;
    for(; is_a_defined_class < total_classes + 1; ++is_a_defined_class)
      if(strcmp(ctored_class, classes[is_a_defined_class].class_name) == 0) break;
    if(is_a_defined_class == total_classes + 1) {
      fprintf(stderr, "\033[1mdeclass.c: \033[33mWARNING\033[0m\033[1m UNDEFINED BEHAVIOR IN FCN: \"%s\", LINE: %d\033[0m\n", __func__, __LINE__);
      fprintf(stderr, " >> EXPECTED OBJECT CTOR FOR MEMBER: \"%s\" IN CLASS: \"%s\"\n", ctored_obj, ctored_class);
      ask_user_whether_to_quit_or_continue_declassification();
      return end;
    }

    // shift & splice in last "&DC_THIS.objName" arg && the ctor functional invocation by assignment
    char *ctor_fcn_assign_ptr = ctor_fcn_assignment, *ctor_append_ptr = appended_ctor_object;
    int ctor_fcn_assign_len   = strlen(ctor_fcn_assignment);
    int ctor_appened_arg_len  = strlen(appended_ctor_object);
    char *endOfFile = end + strlen(end) + ctor_fcn_assign_len; // end of the file + fcnal ctor assign length
    *(endOfFile + 1) = '\0';

    // shift file up then splice in functional ctor invocation by assignment
    // "+ 1" to also shift over the 1st '(' preceding ctor's args
    while(endOfFile - ctor_fcn_assign_len + 1 != end) { 
      *endOfFile = *(endOfFile - ctor_fcn_assign_len); --endOfFile;
    }
    while(*ctor_fcn_assign_ptr != '\0') { // splice in ctor invocation
      *end++ = *ctor_fcn_assign_ptr, *class_size += 1;
      *struct_buff_idx++ = *ctor_fcn_assign_ptr++, *struct_inc += 1;
    }

    // skip to where ought to splice in last "&DC_THIS.objName" ctor arg
    // "+ctor_fcn_assign_len+1" accounts for shifted distance & to move "end" 1 past 1st/last "arg_position"
    // as whether is/isn't an object array ctor
    char *arg_position = (array_ctor) ? FIRST_ARG : LAST_ARG;
    while(end != arg_position + ctor_fcn_assign_len + 1) 
      *struct_buff_idx++ = *end++, *class_size += 1, *struct_inc += 1;
    endOfFile = end + strlen(end) + ctor_appened_arg_len; // end of the file + appended last arg

    // shift file up then splice in last "&DC_THIS.objName" ctor arg
    // + 1 to also shift over char currently in last arg's position to make room for splicing in "&DC_THIS.objName"
    while(endOfFile - ctor_appened_arg_len + 1 != end) { 
      *endOfFile = *(endOfFile - ctor_appened_arg_len); --endOfFile;
    }
    while(*ctor_append_ptr != '\0') { // splice in ctor's last "&DC_THIS.objName" arg
      *end++ = *ctor_append_ptr, *class_size += 1;
      *struct_buff_idx++ = *ctor_append_ptr++, *struct_inc += 1;
    }
  }
  return end;
}

/******************************************************************************
* OBJECT ARRAY DESTRUCTION-MACRO CREATION FUNCTION
******************************************************************************/

// fills 'dtor_array_macro' w/ macro for array object destruction via user's own defined dtor
void mk_dtor_array_macro(char dtor_array_macro[], char *class_name) {
  // sprintf loop to iterate over object array's individual objects to be dtor'd
  sprintf(dtor_array_macro, "#define DC__%s_UDTOR_ARR(DC_ARR) ({\\\n\
  for(int DC__%s_UDTOR_IDX=0;DC__%s_UDTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));\
++DC__%s_UDTOR_IDX)\\\n\t\tif(DC_ARR[DC__%s_UDTOR_IDX].DC_DTR){DC__NOT_%s_(&DC_ARR[DC__%s_UDTOR_IDX]);DC_ARR[DC__%s_UDTOR_IDX].DC_DTR=NULL;}\\\n})",
  class_name, class_name, class_name, class_name, class_name, class_name, class_name, class_name);
}

/******************************************************************************
* USER-DEFINED OBJECT DESTRUCTOR (DTOR) PARSING & SPLICING FUNCTIONS
******************************************************************************/

// shifts up buffer to "splice_here"'s position in order to splice in a dtor
int shiftSplice_dtor_in_buffer(char *dtor, char *splice_here, bool redefined) {
  // check whether object already dtored (prevents redundant dtors,
  // generally occurs when user explicitly invokes a dtor then declass.c 
  // to splice in one at the end of the scope)
  int current_scope = 1;
  char *check_ifdef_already = splice_here;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  while(current_scope > 0 && !redefined) { // doesn't matter if dtor already in scope if object newly redefined
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, check_ifdef_already);
    if(in_token_scope) {
      if(*check_ifdef_already == '{')      --current_scope;
      else if(*check_ifdef_already == '}') ++current_scope;
      if(current_scope <= 0) break;
      // check if dtor already defined in scope & not prior to a return 
      // (return shouldn't play a factor if returning from the same scope but just in case)
      if(current_scope == 1 && is_at_substring(check_ifdef_already, dtor)
        && !is_at_substring(check_ifdef_already + strlen(dtor), "return"))
        return 1; // returns 1 to get past '}' or 'r'eturn
    }
    --check_ifdef_already;
  }
  // if a redundant/duplicate DTOR, return 1 to get past '}' or 'r'eturn
  if(is_at_substring(splice_here - strlen(dtor), dtor)) return 1; 

  // splice in dtor if not already defined in scope at the current position
  char temp[MAX_FILESIZE];
  FLOOD_ZEROS(temp, MAX_FILESIZE);
  sprintf(temp, "%s%s", dtor, splice_here); // sprintf in dtor + buffer after scope ending

  int length = strlen(splice_here);
  FLOOD_ZEROS(splice_here, length); // wipe buffer after out-of-scope
  sprintf(splice_here, "%s", temp); // splice in "dtor" + rest of "temp" buffer of string out of scope
  return strlen(dtor) + 1;          // to get past '}' or "return"
}

// checks whether returned item is the object in question (thus don't invoke class' dtor)
bool object_is_returned(char *returnedItem) {
  while(VARCHAR(*returnedItem))       ++returnedItem; // skip "return"
  while(IS_WHITESPACE(*returnedItem)) ++returnedItem; // skip space after "return" keyword
  if(*returnedItem == '*')            ++returnedItem; // skip over dereferencing operator
  return (is_at_substring(returnedItem, objects[total_objects-1].object_name) 
    && !VARCHAR(*(returnedItem+strlen(objects[total_objects-1].object_name))));
}

// searches for objects in the "?:" return condition results
// "cond_idx" returns idx of object if found, & cond_idx = total_objects if not found
void get_if_else_object_idxs(char *return_cond, int *cond_idx) {
  for(*cond_idx = 0; *cond_idx < total_objects; *cond_idx += 1) {
    char *find_obj = return_cond;
    while(*find_obj != '\0') {
      if(is_at_substring(find_obj, objects[*cond_idx].object_name)
        && (find_obj == return_cond || !VARCHAR(*(find_obj-1))) 
        && !VARCHAR(*(find_obj+strlen(objects[*cond_idx].object_name)))) {
        // trigger invalid-object in calling function IF obj = immortal (all args default to immortal)
        if(objects[*cond_idx].is_immortal) *cond_idx = total_objects;
        return;
      }
     ++find_obj;
   }
  }
}

// returns whether "dtor condition" for when handling "?:" one-line conditionals has been spliced in earlier already
bool unique_dtor_condition(char *returnFrom, char *dtor_condition) {
  // if "max_iter" is hit some joker is writing their entire class without pressing "return",
  // if that joker is you I sincerely hope the thought of undefined behavior gets you jazzed up because
  // lord almighty if so you're in for quite a treat when your compiler gets a stroke trying to run this
  int iterations = 0, max_iter = strlen(dtor_condition) * 4 + 10; 
  while(*returnFrom != '\n' && iterations-- < max_iter) {
    if(is_at_substring(returnFrom, dtor_condition)) return false; // don't splice in redundant dtor conditions
    --returnFrom;
  }
  return true;
}

// checks for "?:" conditional returning 2 different objects to determine which (if any) to dtor
int one_line_conditional(char *cond, bool redefined) {
  char *splice_here = cond;
  int shift_total = 0;
  while(VARCHAR(*cond)) ++cond; // skip "return"
  char condition[MAX_TOKEN_NAME_LENGTH * 2], return_if[MAX_TOKEN_NAME_LENGTH * 2];
  char return_else[MAX_TOKEN_NAME_LENGTH * 2], else_dtor[MAX_TOKEN_NAME_LENGTH * 2], if_dtor[MAX_TOKEN_NAME_LENGTH * 2];
  FLOOD_ZEROS(condition, MAX_TOKEN_NAME_LENGTH * 2); FLOOD_ZEROS(return_if, MAX_TOKEN_NAME_LENGTH * 2); 
  FLOOD_ZEROS(return_else, MAX_TOKEN_NAME_LENGTH * 2); FLOOD_ZEROS(else_dtor, MAX_TOKEN_NAME_LENGTH * 2); 
  FLOOD_ZEROS(if_dtor, MAX_TOKEN_NAME_LENGTH * 2);
  char *write = condition;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  // copy condition, the "if" return, & the "else" return
  while(*cond != '\0') {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, cond);
    if(in_token_scope && *cond == ';') {
      *write = '\0';  break; // end of return's line
    } else if(in_token_scope && *cond == '?') { // start copying first return value
      *write = '\0'; write = return_if;
    } else if(in_token_scope && *cond == ':') {
      *write = '\0'; write = return_else;
    }
    *write++ = *cond++;
  }
  // if return is a "?:" one-line condition
  if(strlen(return_else) > 0) { // found a return in the "?:" format - now determien whether it returns objects
    int if_idx = 0, else_idx = 0;
    get_if_else_object_idxs(return_if, &if_idx);
    get_if_else_object_idxs(return_else, &else_idx);
    if(if_idx == total_objects && else_idx == total_objects) return 0; // no non-pointer mortal objects returned
    // splice in reverse conditions invoking appropriate object destructors if object not being returned
    if(if_idx < total_objects) { 
      if(objects[if_idx].is_class_array)
        sprintf(if_dtor, "if(!(%s)){DC__%s_UDTOR_ARR(%s);}", 
          condition, objects[if_idx].class_name, objects[if_idx].object_name);
      else if(objects[if_idx].is_class_pointer) 
        sprintf(if_dtor, "if(!(%s)){DC__NOT_%s_(%s);}", 
          condition, objects[if_idx].class_name, objects[if_idx].object_name);
      else sprintf(if_dtor, "if(!(%s)){DC__NOT_%s_(&%s);}", 
            condition, objects[if_idx].class_name, objects[if_idx].object_name);
      if(unique_dtor_condition(splice_here, if_dtor)) { // only splice in dtor conditional if not done so already
        shift_total += (shiftSplice_dtor_in_buffer(if_dtor, splice_here, redefined) - 1);
        splice_here += shift_total;
      }
    }
    if(else_idx < total_objects) { 
      if(objects[else_idx].is_class_array)
        sprintf(else_dtor, "if(%s){DC__%s_UDTOR_ARR(%s);}", 
          condition, objects[else_idx].class_name, objects[else_idx].object_name);
      else if(objects[else_idx].is_class_pointer) 
        sprintf(else_dtor, "if(%s){DC__NOT_%s_(%s);}", 
          condition, objects[else_idx].class_name, objects[else_idx].object_name);
      else sprintf(else_dtor, "if(%s){DC__NOT_%s_(&%s);}", 
            condition, objects[else_idx].class_name, objects[else_idx].object_name);
      if(unique_dtor_condition(splice_here, else_dtor)) // only splice in dtor conditional if not done so already
        shift_total += (shiftSplice_dtor_in_buffer(else_dtor, splice_here, redefined) - 1);
    }
  }
  return shift_total;
}

// don't add a dtor prior a '}' brace immdiately following a "return"
int return_then_immediate_exit(char *immediate_exit) {
  int increment = 0;
  while(*immediate_exit != '\0' && *immediate_exit != ';') ++immediate_exit, ++increment; // skip up to ';'
  ++immediate_exit, ++increment;                                                          // skip past ';'
  while(IS_WHITESPACE(*immediate_exit)) ++immediate_exit, ++increment;                    // skip any '\n's/tabs/spaces
  return (*immediate_exit == '}') ? (increment + 1) : 0;                                  // "+1" to skip past '}'
}

// splices object dtor (once object created) into buffer being READ from (picked up later on)
void add_object_dtor(char *splice_here, bool object_being_assigned, bool redefined) {
  if(!objects[total_objects-1].class_has_dtor) return;
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  // immortal objects can only be explicitly dtor'd by user
  bool immortal_object = objects[total_objects-1].is_immortal;
  bool object_pointer = objects[total_objects-1].is_class_pointer;
  bool object_alloced_ptr = objects[total_objects-1].is_alloced_class_pointer;
  bool object_dangling_ctord_ptr = objects[total_objects-1].is_dangling_ctord_ptr;
  bool object_array = objects[total_objects-1].is_class_array;
  int in_scope = 0;
  char dtor[1000];
  FLOOD_ZEROS(dtor, 1000);

  // set up tracker to determine whether or not to destroy elt
  char dtor_flag[MAX_TOKEN_NAME_LENGTH * 2];
  FLOOD_ZEROS(dtor_flag, MAX_TOKEN_NAME_LENGTH * 2);
  while(*splice_here != '\0') { // skip past object declaration
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, splice_here);
    if(in_token_scope && *(splice_here-1) == ';') break;
    ++splice_here;
  }
  // splice in flag for whether object has been destroyed or not
  // >> ALLOCATED & ALL CTOR'D (ALLOC'D OR OTHERWISE) PTRS ALREADY FLAGGED IN MIDDLE OF "MAIN()" & NEAR BOT OF "PARSE_CLASS()"
  // >> ASSIGNED PTRS NOT FLAGGED, AS THEIR FLAG BECOMES NULL WHEN THEIR POINTED TO OBJ'S FLAG DOES (ctor'd ptrs flagged after user ctor)
  //    IE: className *obj2 = &obj1; // "obj2" NOT INITIALLY "2B-DTOR" FLAGGED, AS WILL BE DTOR'D REGARDLESS IF NOT NULL @ END OF ITS SCOPE
  // >> Thus ONLY flag non-allocated, non-assigned, & non-ctor'd ptrs here as initially being NULL, 
  //    & flag non-ctor'd ptrs' dflt "DC_DTR" member after being ctor'd (if they aren't NULL)
  if(!redefined && object_pointer && !object_alloced_ptr && !object_being_assigned && !object_dangling_ctord_ptr) {
    sprintf(dtor_flag, " %s=NULL;", objects[total_objects-1].object_name);
  // if a ptr thats ctord but not allocated (allocated version of this taken care of in middle of "main()" & near bot of "parse_class()"")
  } else if(!redefined && object_pointer && object_dangling_ctord_ptr) {
    sprintf(dtor_flag, " if(%s){%s->DC_DTR=&DC__NDTR;}", objects[total_objects-1].object_name, objects[total_objects-1].object_name);
  // non-array && non-ptr
  } else if(!redefined && !object_array && !object_pointer) {
    sprintf(dtor_flag, " %s.DC_DTR=&DC__NDTR;", objects[total_objects-1].object_name);
  }
  splice_here += shiftSplice_dtor_in_buffer(dtor_flag, splice_here, redefined);

  // determine which type of destructor to splice in (single, ptr, or array (array's indiv cells have their flags checked in the macro))
  if(object_array)
    sprintf(dtor, "DC__%s_UDTOR_ARR(%s);\n", objects[total_objects-1].class_name, objects[total_objects-1].object_name);
  else if(object_pointer) // "dangling" ptrs can't access members, thus dtors the entire ptr as NULL
    sprintf(dtor, "if(%s&&%s->DC_DTR){DC__NOT_%s_(%s);%s=NULL;}\n", objects[total_objects-1].object_name, objects[total_objects-1].object_name,
      objects[total_objects-1].class_name, objects[total_objects-1].object_name, objects[total_objects-1].object_name);
  else 
    sprintf(dtor, "if(%s.DC_DTR){DC__NOT_%s_(&%s);%s.DC_DTR=NULL;}\n", objects[total_objects-1].object_name,
      objects[total_objects-1].class_name, objects[total_objects-1].object_name, objects[total_objects-1].object_name);

  // find where current scope ends
  while(*splice_here != '\0' && in_scope >= 0) {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, splice_here);
    // check whether entering/exiting a scope
    if(in_token_scope) {
      if(*splice_here == '{') ++in_scope;
      else if(*splice_here == '}') {
        --in_scope;
        if(in_scope < 0) {
          if(!immortal_object) shiftSplice_dtor_in_buffer(dtor, splice_here, redefined);
          return;
        }
      } else if(is_at_substring(splice_here,"return") && !immortal_object
        && !VARCHAR(*(splice_here-1)) && !VARCHAR(*(splice_here+strlen("return")))) {
        // if !*DTOR_RETURN, splice in dtors as needed if return case: "return (condition) ? object1 : object 2;"
        int one_line_cond = (*DTOR_RETURN) ? 0 : one_line_conditional(splice_here, redefined);
        if(one_line_cond > 0) {
          splice_here += one_line_cond;
          int immediate_exit = return_then_immediate_exit(splice_here);
          if(immediate_exit > 0) { --in_scope; if(in_scope < 0) return; splice_here += immediate_exit; }
          ++splice_here;
          continue;
        }
        // if !*DTOR_RETURN, don't dtor an object being returned
        if(!(*DTOR_RETURN) && object_is_returned(splice_here)) { 
          int immediate_exit = return_then_immediate_exit(splice_here);
          if(immediate_exit > 0) { --in_scope; if(in_scope < 0) return; splice_here += immediate_exit; }
          ++splice_here; 
          continue; 
        } 
        // splice in DTOR prior to "return", NOT changing scope status
        splice_here += shiftSplice_dtor_in_buffer(dtor, splice_here, redefined);
        // don't add a redundant dtor prior a '}' brace immediately following a "return"
        int immediate_exit = return_then_immediate_exit(splice_here);
        if(immediate_exit > 0) { --in_scope; if(in_scope < 0) return; splice_here += immediate_exit; }
      } else if(*splice_here == '~') { // check is user explicitly invoked destructor, ie: "~objName();"
        if(is_at_substring(splice_here + 1, objects[total_objects-1].object_name)
          && !VARCHAR(*(splice_here + 1 + strlen(objects[total_objects-1].object_name)))) { // user destroyed obj
          char *whiteout = splice_here;
          while(*whiteout != '\0' && *whiteout != ';' && *whiteout != ':')                  // whitespace dtor invocation
            *whiteout++ = ' ';
          if(*whiteout == ';') *whiteout = ' '; // whitespace last ';' (not ':' if in "?:" conditional)
          splice_here += shiftSplice_dtor_in_buffer(dtor, splice_here, redefined);
        }
      } else if(is_at_substring(splice_here, dtor) && in_scope == 0) return; // no need to dtor if already in EXACT same scope as declaration
    }
    ++splice_here;
  }
}

/******************************************************************************
* OBJECT METHOD PARSER
******************************************************************************/

// parse object method invocations -- 'method_words' only meaningful when 'is_nested_method' == true
int parse_method_invocation(char *s, char *NEW_FILE, int *j, bool is_nested_method, char method_words[][MAX_TOKEN_NAME_LENGTH]) {
  char *p = s, first_char = *s, new_fcn_call[MAX_TOKEN_NAME_LENGTH];
  FLOOD_ZEROS(new_fcn_call, MAX_TOKEN_NAME_LENGTH);
  int method_name_size = 0;
  if(!(VARCHAR(*p)) && (VARCHAR(*(p + 1)))) {                                  // may be object
    p++;
    for(int i = 0; i < total_objects; ++i) {                                   // check for each object
      int len = strlen(objects[i].object_name);
      if(strlen(p) > len && is_at_substring(p, objects[i].object_name)) {      // found object
        int invoker_size = is_method_invocation(p + len);
        if(invoker_size == 0) continue;                                        // no invocation notation (no '.' nor '->')
        char invoked_member_name[MAX_TOKEN_NAME_LENGTH];
        get_invoked_member_name(p + len + invoker_size, invoked_member_name);  // get member name
        if(invoked_member_is_method(invoked_member_name, objects[i].class_name, is_nested_method)) { // member = method
          while((invoker_size = is_method_invocation(p)) > 0 || VARCHAR(*p)) { // move p to after object & method names
            if(invoker_size == 0) invoker_size = 1; // VARCHAR
            p += invoker_size, method_name_size += invoker_size;
          }
          method_name_size++;                                                  // for 1st char ('%c' in sprintf below)
          sprintf(new_fcn_call, "%cDC_%s_%s", first_char, objects[i].class_name, invoked_member_name);

          // whether method is invoked within another method, but splice in either way
          if(is_nested_method)
            splice_in_prepended_NESTED_method_name(new_fcn_call, p, i, NEW_FILE, &method_name_size, method_words);
          else
            splice_in_prepended_method_name(new_fcn_call, p, i, NEW_FILE, j, &method_name_size, method_words);
          break;
        }
      }
    }
  } else method_name_size = 0;
  return method_name_size;
}

// splice in prepended method name to buffer via an index subscript
void splice_in_prepended_method_name(char*new_fcn_call,char*p,int i,char*NEW_FILE,int*j,
                                     int*method_name_size,char method_words[][MAX_TOKEN_NAME_LENGTH]) {
  char objectName[MAX_TOKEN_NAME_LENGTH * 2], objectChain[MAX_TOKEN_NAME_LENGTH * 2]; // 'objectName' refers to outermost object in 'objectChain
  bool objectName_is_pointer = false;
  FLOOD_ZEROS(objectName, MAX_TOKEN_NAME_LENGTH * 2); FLOOD_ZEROS(objectChain, MAX_TOKEN_NAME_LENGTH * 2); 
  get_object_name(objectName, objectChain, p, i, method_words, false, &objectName_is_pointer);

  // remove invoking object's container object chain-prefix from 'NEWFILE' (if present)
  char *rmv_buffer_objectChain = &NEW_FILE[*j];
  rmv_excess_buffer_objectChain(objectName, objects[i].object_name, objectChain, rmv_buffer_objectChain, new_fcn_call);
  *j = strlen(NEW_FILE);

  // splice in prefixed method name & add invoker's address to end of arguments
  APPEND_STR_TO_NEW_FILE(new_fcn_call);
  while(*p != '\0' && *p != ')') NEW_FILE[(*j)++] = *p++, (*method_name_size)++;
  if(*(p - 1) != '(') NEW_FILE[(*j)++] = ',', NEW_FILE[(*j)++] = ' ';
  if(objectName_is_pointer) sprintf(&NEW_FILE[*j], "%s", objectChain);  // splice in ptr obj address
  else                      sprintf(&NEW_FILE[*j], "&%s", objectChain); // splice in val obj address
  *j = strlen(NEW_FILE);
}

// splice in prepended method name to buffer via a pointer
void splice_in_prepended_NESTED_method_name(char*new_fcn_call,char*p,int i,char*method_buff_idx,
                                            int*method_name_size,char method_words[][MAX_TOKEN_NAME_LENGTH]){
  char objectName[MAX_TOKEN_NAME_LENGTH * 2], objectChain[MAX_TOKEN_NAME_LENGTH * 2]; // 'objectName' refers to outermost object in 'objectChain
  bool objectName_is_pointer = false;
  FLOOD_ZEROS(objectName, MAX_TOKEN_NAME_LENGTH * 2); FLOOD_ZEROS(objectChain, MAX_TOKEN_NAME_LENGTH * 2);
  get_object_name(objectName, objectChain, p, i, method_words, true, &objectName_is_pointer);

  // remove invoking object's container object chain-prefix from 'method_buff_idx' (if present)
  rmv_excess_buffer_objectChain(objectName, objects[i].object_name, objectChain, method_buff_idx, new_fcn_call);
  while(*method_buff_idx == '\0') --method_buff_idx; // move back to 1st-'\0' sans-chain position
  ++method_buff_idx;                                 // move up to '\0'
  // if still need to remove 'this->' prefix
  if(is_at_substring(method_buff_idx - 6, "this->")) 
    for(int k = 0; k < 6; ++k) *method_buff_idx-- = '\0';

  // if the outermost 'objectName' object in 'objectChain' is itself also a member of the current class
  for(int idx = 0; idx < classes[total_classes].total_members; ++idx) {
    if(classes[total_classes].member_names[idx][0] == 0) continue; // struct member -- disregard
    if(strcmp(classes[total_classes].member_names[idx], objectName) == 0) {
      FLOOD_ZEROS(objectName, MAX_TOKEN_NAME_LENGTH * 2);                   // empty out object name
      sprintf(objectName, "(this->%s)", objectChain); // write prefixed-objectChain to objectName 
      FLOOD_ZEROS(objectChain, MAX_TOKEN_NAME_LENGTH * 2);                  // empty objectChain
      strcpy(objectChain, objectName);                // refill object with it's prefixed self
      break;
    }
  }

  // add prefixed method name
  sprintf(method_buff_idx, "%s", new_fcn_call); 
  method_buff_idx += strlen(method_buff_idx);
  // copy method argument & prefix 'this->' for any local members w/in
  p += prefix_local_members_and_cpy_method_args(p, method_buff_idx, method_words, method_name_size, ')');
  method_buff_idx += strlen(method_buff_idx);
  if(*(p - 1) != '(') *method_buff_idx++ = ',', *method_buff_idx++ = ' ';
  if(objectName_is_pointer) sprintf(method_buff_idx, "%s", objectChain);  // splice in ptr obj address
  else                      sprintf(method_buff_idx, "&%s", objectChain); // splice in val obj address
}

// removes the excess front of the objectChain from 'buffer', occuring if 
// the object invoking method is also invoked by another container object
void rmv_excess_buffer_objectChain(char*objectName,char*invokingObject,char*objectChain,char*buffer,char*new_fcn_call){
  if(strcmp(objectName, invokingObject) != 0) { // again, 'objectName' refers to outermost object in 'objectChain'
    // to get rid of "first_char" (possible method invoker) left in from "parse_method_invocation()"
    char *temp = new_fcn_call;
    while(*temp != '\0') *temp = *(temp + 1), ++temp;
    // removes excess/left-over objectChain from 'buffer'
    char objectChain_clone[MAX_TOKEN_NAME_LENGTH * 2]; // used to get length values to rmv from 'buffer' w/o the altering original objectChain
    FLOOD_ZEROS(objectChain_clone, MAX_TOKEN_NAME_LENGTH * 2);
    strcpy(objectChain_clone, objectChain);
    char *excess_chain = &objectChain_clone[strlen(objectChain_clone) - 1];    // traverse chain up to first invoker
    if(*excess_chain == ']') while(*(excess_chain + 1) != '[') --excess_chain; // skip first subscript
    while(VARCHAR(*excess_chain)) --excess_chain;                              // skip 'invokingObject' method invoker
    *excess_chain = '\0';
    int chain_length = strlen(objectChain_clone);                              // how many chars to rmv from 'buffer'
    for(int k = 0; k < chain_length; ++k) *buffer-- = '\0';                    // rmv excess chained objects
    *buffer = '\0';
  }
}

/******************************************************************************
* OBJECT METHOD PARSER HELPER FUNCTIONS
******************************************************************************/

// returns length of '.' or '->' invocation (0 if none) + length of array 
// subscript (if present) for invocation by a class w/in a class array
int is_method_invocation(char *p) {
  #define after_arr(p) (p + array_arg_size)
  int invoker_size = 0, array_arg_size = 1;
  // has (or is in) an array subscript
  char *open_bracket = p, *close_bracket = p; 
  while(*open_bracket != '\0' && no_overlap(*open_bracket, "[;\n")) --open_bracket;
  while(*close_bracket != '\0' && no_overlap(*close_bracket, "];\n")) ++close_bracket, ++array_arg_size;
  if(*open_bracket != '[' || *close_bracket != ']') array_arg_size = 0;

  // period member invocation
  if(*p == '.' || *after_arr(p) == '.') return array_arg_size + 1;      
  while(*after_arr(p) != '\0' && *after_arr(p) == ' ') invoker_size++, p++;

  // arrow (may have whitespace on either side)
  if((*after_arr(p) == '-' && *(after_arr(p)+1) == '>') || (*after_arr(p) == '>' && *(after_arr(p)-1) == '-')) {
    p += 2, invoker_size += 2;
    while(*after_arr(p) != '\0' && *after_arr(p) == ' ') invoker_size++, p++;
  } else invoker_size = array_arg_size = 0; // none
  return invoker_size + array_arg_size;
  #undef after_arr
}

// return whether object's invoked member is a class method
bool invoked_member_is_method(char *invoked_member_name, char *class_name, bool is_nested_method) {
  // '+ is_nested_method' b/c nested methods processed prior to total_classes++
  for(int i = 0; i < total_classes + is_nested_method; ++i) 
    if(strcmp(classes[i].class_name, class_name) == 0)
      for(int j = 0; j < classes[i].total_methods; ++j)
        if(strcmp(classes[i].method_names[j], invoked_member_name) == 0) return true;
  return false;
}

 // return object's invoked 'class' member
void get_invoked_member_name(char *member_ptr, char *invoked_member_name) {
  int i = 0;
  while(*member_ptr != '\0' && VARCHAR(*member_ptr)) 
    invoked_member_name[i++] = *member_ptr++;
  invoked_member_name[i] = '\0';
}

// returns outmost object's name, the entire object invocation chain (w/ prefixed array
// subscripts as needed) as well as whether the outermost object is a pointer
// 'is_nested_object' indicates parsing a nested method-within-a-method invocation
void get_object_name(char*outerMost_objectName,char*objectChain,char*buffer,int i,
                     char method_words[][MAX_TOKEN_NAME_LENGTH],bool is_nested_object,bool*objectName_is_pointer) {
  // * outerMost_objectName is the name - w/o subscripts - of the outermost object leading the object chain of invocation
  // * the objectChain consists of an optional outer container object that contains the method-invoking
  //   object, as well as any potential array subscripts for either the container &/or invoking object.

  // find the chain's start, going past array subscripts, invocation notations, & object names.
  // assumes array subscripts are correct & user follows caveat (8) (will splice 'this->' as needed in subscript though)
  char *chain_head = buffer - 1, *chain_tail = buffer - 1;
  int in_a_subscript = 0;
  while(no_overlap(*chain_head, "\n;{}") && (VARCHAR(*chain_head) || !no_overlap(*chain_head, "[] .>"))) {
    if(*chain_head == ']') {                                         // don't parse subscripts yet - skip over
      ++in_a_subscript, --chain_head;                                // move past current subscript brace
      while(in_a_subscript > 0) {
        if(*chain_head == ']')       ++in_a_subscript;
        else if(*chain_head == '[')  --in_a_subscript;
        --chain_head;  
      } 
    }
    if(*chain_head == '>' && *(chain_head - 1) == '-') --chain_head; // move past arrow notation
    --chain_head;
  }
  while(!VARCHAR(*chain_head)) ++chain_head;                         // move up to the first object name

  // move chain tail past the method name ('buffer' starts at '(' after method name) to end of invocation
  char invoc_punc[MAX_TOKEN_NAME_LENGTH];      // stores invocation punctuation IN REVERSE (ie '->' stored as '>-')
  FLOOD_ZEROS(invoc_punc, MAX_TOKEN_NAME_LENGTH);
  int index = 0;
  while(VARCHAR(*chain_tail)) --chain_tail;                          // skip method name
  while(!VARCHAR(*chain_tail) && no_overlap(*chain_tail, "[]"))      // cpy/skip method invocation punct
    invoc_punc[index++] = *chain_tail--;
  ++chain_tail;                                                      // move to 1 char past the chain's end
  invoc_punc[index] = '\0'; // determines 'chainless'/single-object invocation ptr status

  // save object method invocation chain
  index = 0;
  while(chain_head != chain_tail) objectChain[index++] = *chain_head++;
  objectChain[index] = '\0';

  // save the outermost object's name - w/o (if present) array subscript
  char *outermost = objectChain;
  index = 0;
  while(VARCHAR(*outermost)) outerMost_objectName[index++] = *outermost++;
  outerMost_objectName[index] = '\0';

  // if nested, find any local members w/in array subscript to prefix 'this->'
  if(is_nested_object) {
    char prefixed_objectChain[MAX_TOKEN_NAME_LENGTH * 2];
    FLOOD_ZEROS(prefixed_objectChain, MAX_TOKEN_NAME_LENGTH * 2);
    prefixed_objectChain[0] = '\0';
    char *read_chain = objectChain, *write_chain = prefixed_objectChain;
    while(*read_chain != '\0') {
      if(*read_chain == '[') {
        int filler_int_arg = 0;
        // repurpose method argument parser/copier to handle array subscript instead
        read_chain += prefix_local_members_and_cpy_method_args(read_chain,write_chain,method_words,&filler_int_arg,']');
        write_chain += strlen(write_chain);
      }
      if(*read_chain != '\0') *write_chain++ = *read_chain++;
    }
    *write_chain = '\0';
    FLOOD_ZEROS(objectChain, MAX_TOKEN_NAME_LENGTH * 2); // save prefixed object chain as the only object chain
    strcpy(objectChain, prefixed_objectChain);
  }

  // determine whether outermost object is a ptr via its method invocation punctuation
  int tries = 0;
  char *ptr_status;
  while(tries < 2) { // try searching 'objectChain' then 'invoc_punc' for pointer indicators
    ptr_status = (tries == 0) ? objectChain : invoc_punc;
    while(*ptr_status != '\0' && *ptr_status != '.') {
      if(*ptr_status == '[') while(*ptr_status != '\0' && *ptr_status != ']') ++ptr_status; // skip array subscript
      if(*ptr_status == '>' && *(ptr_status + 1) == '-') { // was stored backwards!
        *objectName_is_pointer = true;
        return;
      }
      ++ptr_status;
    }
    ++tries;
  }
  *objectName_is_pointer = false;
}

// prefixes local members with 'this->' & cpys entire method arg to 'write_to_buffer'
int prefix_local_members_and_cpy_method_args(char*end,char*write_to_buffer,char method_words[][MAX_TOKEN_NAME_LENGTH],
                                             int*buffer_length,char delimiter) {
  int end_increment = 0;
  char *word_start = end + 1, *findArrow;
  while(*end != '\0' && *end != delimiter) { // copy method arguments
    // search the invoked local method's args for any local members
    if(!VARCHAR(*end) && *end != '.' && (*end != '>' || *(end-1) != '-') && VARCHAR(*(end+1))) {
      if(IS_WHITESPACE(*end)) {                       // don't prefixed an invoked member with 'this'
        findArrow = end;
        while(IS_WHITESPACE(*findArrow)) --findArrow; // screen for arrow invocation
        if(*findArrow == '>' && *(findArrow-1) == '-') {
          *write_to_buffer++ = *end++, *buffer_length += 1, ++end_increment;
          continue;
        }
      }
      word_start = end + 1; 
    } else if(VARCHAR(*end) && !VARCHAR(*(end+1))) {                          // at argument word's end
      char argument[MAX_TOKEN_NAME_LENGTH];
      FLOOD_ZEROS(argument, MAX_TOKEN_NAME_LENGTH);
      int idx = 0;
      while(word_start != end + 1) argument[idx++] = *word_start++;           // copy argument word
      argument[idx++] = '\0';
      if(not_in_method_words(method_words, idx, argument))                    // argument != redefined variable
        for(int i = 0; i < classes[total_classes].total_members; ++i)
          if(strcmp(classes[total_classes].member_names[i], argument) == 0) { // argument = local member
            splice_in_this_arrowPtr(write_to_buffer);
            write_to_buffer += strlen(write_to_buffer);
            break; 
          }
    }
    *write_to_buffer++ = *end++, *buffer_length += 1, ++end_increment;        // copy-current & traverse-next char in arg
  }
  return end_increment;
}

/******************************************************************************
* STORE OBJECT INFORMATION
******************************************************************************/

// stores an object's name & associated class name/type in global struct
// and checks whther currently at a so-called "dummy ctor" which only returns an arg
// total_classes_increment: -1 = arg (no dtor) 
//                           0 = in fcn (total_classes)      (dtor if not returned) 
//                           1 = in method (total_classes+1) (dtor if not returned)
bool store_object_info(char *s, int total_classes_increment, bool *dummy_ctor) {
  bool not_an_arg = true, is_fcn_assignment = false, has_dtor = false, is_alloced_class_pointer = false;
  char *q = s, *p = s, object_name[MAX_TOKEN_NAME_LENGTH], class_type_name[MAX_TOKEN_NAME_LENGTH];
  FLOOD_ZEROS(object_name, MAX_TOKEN_NAME_LENGTH); 
  FLOOD_ZEROS(class_type_name, MAX_TOKEN_NAME_LENGTH);
  int i = 0, j = 0;
  *dummy_ctor = false;

  while(*q != '\0' && *q != ';') if(*q++ == ')') { not_an_arg = false; break; } // determine if arg
  // determine whether object is assigned a value by a fcn 
  // OR initialized by user-defined ctor
  if(!not_an_arg) {
    q = s;
    while(*q != '\0' && no_overlap(*q, ";,)")) if(*q++ == '(') { is_fcn_assignment = true; break; }
    if(is_fcn_assignment) {
      q = s;
      while(*q != '\0' && no_overlap(*q, ";,)")) if(*q++ == '(' || *q++ == '=') { is_fcn_assignment = true; break; }
      not_an_arg = is_fcn_assignment; // an obj assigned a fcn return value is NOT an obj declaration as an arg
    }
  }

  // determine whether object is prefixed with "immortal" keyword (never invokes class dtor if tagged "immortal")
  bool is_immortal = false;
  if(*IMMORTALITY) {
    char *check_immortal = s - 1;                                    // start 1 position before class name type
    while(IS_WHITESPACE(*check_immortal) && *check_immortal != '\n') // move past the space btwn "immortal' '<type>"
      --check_immortal;
    while(VARCHAR(*check_immortal)) --check_immortal;                // move in front of possible "immortal" keyword
    ++check_immortal;                                                // move to first letter
    if(is_at_substring(check_immortal, "immortal") && !VARCHAR(*(check_immortal + strlen("immortal"))))
      is_immortal = true;
  }

  // objects passed as arguments are default considered immortal 
  // (no dtor for them in fcn passed to, only in fcn passed from)
  if(!not_an_arg) is_immortal = true;

  // get object's class name & other attributes
  while(!IS_WHITESPACE(*p) && no_overlap(*p, "*(")) // skip class_type declaration
    class_type_name[i++] = *p++; 
  if(IS_WHITESPACE(*p)) ++p; // move to object name (checks here in case "className*objName;")
  bool is_class_pointer = false, is_class_array = false;

  // check if currently at a so-called "dummy ctor" being invoked to return an object
  if(*p == '(') {
    *dummy_ctor = true;
    return true;
  }

  if(*p == '*') is_class_pointer = true, p++;        // check if object == class pointer
  // determine whether pointer object have been alloc'd memory (if so init w/ dflt vals)
  if(is_class_pointer && is_fcn_assignment) {
    char *check_alloc = p; 
    while(*check_alloc != '=' && *check_alloc != ';') ++check_alloc; // move to to the assignment
    if(*check_alloc == '=') { // is a potential assignment & not a ctor (as "is_fcn_assignment" also repns ctors)
      ++check_alloc;                                    // skip '='
      while(IS_WHITESPACE(*check_alloc)) ++check_alloc; // skip optional space between '=' & allocation
      if(is_an_alloc_fcn(check_alloc))                  // check if pointer being allocated memory
        is_alloced_class_pointer = true;
    }
  }

  while(VARCHAR(*p)) object_name[j++] = *p++;           // store object's name, class, & ptr status
  if(*p == '[') is_class_array = true;                  // check if object == class array
  object_name[j] = '\0', class_type_name[i] = '\0';
  if(strlen(object_name) == 0 || class_type_name[strlen(class_type_name)-1] == ',') return false; // prototype fcn arg
  strcpy(objects[total_objects].object_name, object_name);
  strcpy(objects[total_objects].class_name, class_type_name);
  objects[total_objects].is_class_pointer = is_class_pointer;
  objects[total_objects].is_alloced_class_pointer = is_alloced_class_pointer;
  objects[total_objects].is_class_array = is_class_array;
  objects[total_objects].is_immortal = is_immortal;

  // determine whether object's class uses a dtor (default false if object == an arg tho)
  if(total_classes_increment != -1 && not_an_arg) // object declared != arg
    for(int k = 0; k < total_classes + total_classes_increment; ++k)
      if(strcmp(classes[k].class_name, class_type_name) == 0) {
        has_dtor = classes[k].class_has_dtor;
        break;
      }
  objects[total_objects].class_has_dtor = has_dtor;
  total_objects++;

  // don't pre-init dflt vals to arguments 
  // (nor pointers w/o allocated memory, but such is handled externally)
  return not_an_arg;
}

/******************************************************************************
* PARSE CLASS HELPER FUNCTIONS
******************************************************************************/

// stores a new class instance's default stats in the global "classes" struct
void initialize_new_class_default_properties(char *class_name) {
  strcpy(classes[total_classes].class_name, class_name);
  classes[total_classes].total_methods = 0, classes[total_classes].total_members = 1;
  classes[total_classes].total_uctors = 0;
  classes[total_classes].class_has_alloc = false, classes[total_classes].class_has_dtor = false;
  classes[total_classes].class_has_ctor = false, classes[total_classes].class_has_ctor_args = false;
  classes[total_classes].member_value_user_ctor[0][0] = 0, classes[total_classes].member_value_user_ctor[0][1] = '\0';
  classes[total_classes].member_object_class_name[0][0] = 0, classes[total_classes].member_object_class_name[0][1] = '\0';
  classes[total_classes].member_is_immortal[0] = classes[total_classes].member_value_is_alloc[0] = false;
  classes[total_classes].member_is_array[0] = false;
  classes[total_classes].member_is_pointer[0] = true;
  strcpy(classes[total_classes].member_names[0], "object_has_been_destroyed_flag");
}

// returns whether struct is definition (true) or variable declaration (false)
bool is_struct_definition(char *end) {
  char *brace = end, *newline = end;
  while(*newline != '\0' && *newline != '\n') ++newline;
  while(*brace != '\0' && *brace != '{') if(brace++ == newline) return false;
  return true;
}

// get class (now struct) name
void get_class_name(char *s, char *class_name) {
  char *p = s, *name = class_name;
  while(*p != '\0' && *p++ != ' ');                                         // skip 'class '
  while(*p != '\0' && *p != ' ' && *p != '{' && *p != '\n') *name++ = *p++; // copy class (becoming struct) name 
  *name = '\0';
}

// checks whether or not class already has a user-defined ctor/dtor, & throws error if so
// (unless is a ctor & "#define DECLASS_NCOLA" was not found, then "confirm_only_valid_cola_overloads" 
// checks to confirm only arg-length based overloads were made & throws an error if needed)
void confirm_only_one_cdtor(char *class_name, char *structor_type, bool possible_dtor) {
  if((possible_dtor && classes[total_classes].class_has_dtor) || (!possible_dtor && classes[total_classes].class_has_ctor && NO_COLA_PARSER)) {
    fprintf(stderr, "\n >> declass.c: \033[1m\033[31mERROR\033[0m MORE THAN 1 %s FOR CLASSNAME \"%s\" FOUND!", structor_type, class_name);
    fprintf(stderr, "\n >> NO FUNCTION OVERLOADING");
    if(!possible_dtor && classes[total_classes].class_has_ctor && NO_COLA_PARSER)
      fprintf(stderr, " (\"#define DECLASS_NCOLA\" WAS DETECTED!)");
    if(MORTAL_ERRORS) ask_user_whether_to_quit_or_continue_declassification();
    else {
      fprintf(stderr, " >> Terminating Declassifier.\n");
      exit(EXIT_FAILURE);
    }
  }
}

// confirm only COLA (arg-length based function/macro overloads) have been made for ctors
void confirm_only_valid_cola_overloads(char *class_name) {
  for(int i = 0, j; !NO_COLA_PARSER && i < classes[total_classes].total_uctors - 1; ++i)
    for(j = i + 1; j < classes[total_classes].total_uctors; ++j)
      if(classes[total_classes].class_uctor_arg_lengths[i] == classes[total_classes].class_uctor_arg_lengths[j]) {
        fprintf(stderr, "\n >> declass.c: \033[1m\033[31mERROR\033[0m INVALID DUPLICATE CTOR ARGUMENT-LENGTH \"COLA\" OVERLOAD DETECTED!\n");
        fprintf(stderr, " >> FOUND IN CLASSNAME \"%s\"\n", class_name);
        fprintf(stderr, " >> ONLY FUNCTIONS & CTORS W/ DIFFERING #'S OF ARGUMENTS CAN BE \"COLA\" OVERLOADED!\n");
        if(MORTAL_ERRORS) ask_user_whether_to_quit_or_continue_declassification();
        else {
          fprintf(stderr, " >> Terminating Declassifier.\n");
          exit(EXIT_FAILURE);
        }
      }
}

// count the # args in any given ctor (used to design dummy/array object ctor macros for COLA-overloaded user-def'd ctors)
int count_class_cdtor_args(char *r) {
  bool in_a_string = false, in_a_char = false, in_token_scope = true;
  int in_arg_scope = 1, arg_total = 1;
  ++r; // move "r" to 1st arg in fcn & past opening '('
  while(*r != '\0' && in_arg_scope > 0) {
    account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, r);
    if(in_token_scope && *r == '(')      ++in_arg_scope;
    else if(in_token_scope && *r == ')') --in_arg_scope;
    if(in_arg_scope <= 0) break;
    if(in_token_scope && *r == ',') ++arg_total; // at an arg
    ++r;
  }
  --r; // check whether an empty arg list
  while(IS_WHITESPACE(*r)) --r;
  return (*r == '(') ? 0 : arg_total;
}

// parse & prepend function name w/ class (now struct) name, & determine 
// whether method is user-defined class constructor/destructor
void get_prepended_method_name(char*s,char*class_name,char*prepended_method_name,bool*method_is_ctor,bool*method_is_dtor){
  int total_utor_args = 0; // for cola.c overloading
  char method_name[MAX_TOKEN_NAME_LENGTH], cdtor_name[MAX_TOKEN_NAME_LENGTH], structor_name[30], structor_type[30];
  char structor_invoker[MAX_TOKEN_NAME_LENGTH * 2], bad_code_buffer[BAD_CODE_BUFFER_LENGTH];
  FLOOD_ZEROS(method_name, MAX_TOKEN_NAME_LENGTH);   FLOOD_ZEROS(cdtor_name, MAX_TOKEN_NAME_LENGTH);
  FLOOD_ZEROS(structor_name, 30); FLOOD_ZEROS(structor_type, 30);
  FLOOD_ZEROS(structor_invoker, MAX_TOKEN_NAME_LENGTH * 2);
  char *p = s, *q, *name = method_name, *cdtor = cdtor_name;
  *method_is_ctor = false, *method_is_dtor = false;
  while(*p != '\0' && IS_WHITESPACE(*p)) p++; p++; // skip tab

  // check for whether method is user-defined class dtor or ctor (typeless fcn 
  // w/ same name as class, prefixed w/ '~' if its a destructor (dtor))
  bool possible_dtor = (*(p-1) == '~');
  if(possible_dtor) {
    strcpy(structor_name, "DC__destructor");
    strcpy(structor_type, "DESTRUCTOR");
    sprintf(structor_invoker, "DC__NOT_%s_", class_name);
    q = p;
  } else {            // possible constructor
    strcpy(structor_name, "DC__constructor");
    strcpy(structor_type, "CONSTRUCTOR");
    sprintf(structor_invoker, "DC_%s_", class_name);
    q = p - 1;
  }

  while(*q != ' ' && *q != '(') *cdtor++ = *q++;         // "type" if *q == ' ' && "class_name" if == '('
  *cdtor = '\0';
  if(*q == '(' && strcmp(cdtor_name, class_name) == 0) { // no type & fcn name == class_name: method = ctor/dtor
    confirm_only_one_cdtor(class_name, structor_type, possible_dtor);
    strcpy(prepended_method_name, structor_invoker);
    strcpy(classes[total_classes].method_names[classes[total_classes].total_methods], structor_name);
    if(possible_dtor) {
      *method_is_dtor = classes[total_classes].class_has_dtor = true;
      if(count_class_cdtor_args(q) > 0) { // DTOR __CANNOT__EVER__ take args!
        FLOOD_ZEROS(bad_code_buffer, BAD_CODE_BUFFER_LENGTH);
        get_invalid_code_snippet(s, q, bad_code_buffer);
        throw_fatal_error_dtor_has_args(class_name, __LINE__, bad_code_buffer);
      }
    } else {
      *method_is_ctor = classes[total_classes].class_has_ctor = true;
      total_utor_args = count_class_cdtor_args(q); // retrieve & save the number of udef'd args for udef'd ctor
      classes[total_classes].class_uctor_arg_lengths[classes[total_classes].total_uctors] = total_utor_args;
      ++ classes[total_classes].total_uctors; // increment # of user-defined ctors
      if(total_utor_args > 0) classes[total_classes].class_has_ctor_args = true;
      confirm_only_valid_cola_overloads(class_name);
    }

  // is not ctor -- continue cpying method name
  } else { 
    while(*p != '\0' && *p++ != ' ');                                    // skip type
    while(*p != '\0' && VARCHAR(*p)) *name++ = *p++;                     // copy name
    *name = '\0';
    sprintf(prepended_method_name, "DC_%s_%s", class_name, method_name); // className_'function name'
    // store method info in global class struct's instance of the current class
    strcpy(classes[total_classes].method_names[classes[total_classes].total_methods], method_name);
  }
  classes[total_classes].total_methods += 1;
}

// returns a member's initialized value (0 by default) & returns how far back name is after value initialization
int get_initialized_member_value(char *member_end) {
  char *start_of_val;
  int i = 0, distance_back = 0;
  int len = classes[total_classes].total_members;
  --member_end, ++distance_back;
  while(*member_end != '\0' && no_overlap(*member_end, "=;\n")) --member_end, ++distance_back; // find '='
  if(*member_end == '=') {                                                                     // is initialized value
    start_of_val = member_end + 1;
    while(IS_WHITESPACE(*start_of_val)) start_of_val++;                                        // find start of value
    for(; *start_of_val != ';'; i++, ++start_of_val)                                           // copy initialized value
      classes[total_classes].member_values[len][i] = *start_of_val;
    classes[total_classes].member_values[len][i] = '\0';

    // determine whether obj is a ptr
    char *check_ptr = member_end;
    while(no_overlap(*check_ptr, "\n;*")) --check_ptr;
    bool is_obj_ptr = (*check_ptr == '*');
    
    // determine whether initialized member value was a form of memory allocation
    classes[total_classes].member_value_is_alloc[len] = false;
    char *is_alloc = classes[total_classes].member_values[len];
    char *value_front = is_alloc;
    while(*is_alloc != '\0') {
      if(is_an_alloc_fcn(is_alloc) && (is_alloc == value_front || !VARCHAR(*(is_alloc-1)))) {
        classes[total_classes].member_value_is_alloc[len] = true;
        classes[total_classes].class_has_alloc = true;
        break;
      }
      ++is_alloc;
    }

    /* 
     * determine whether initialized member value was a user-defined ctor array invocation
     * wherease single-object ctors use a function that can return an object, arrays rely on
     * invoking a macro (thus doesn't work for brace initialization) so splice out the macro 
     * in the DFLT fcn for the class && invoke the user-defined ctor outside of the object array's
     * brace initialization
    */
    char obj_arr_ctor_val[MAX_TOKEN_NAME_LENGTH * 3]; FLOOD_ZEROS(obj_arr_ctor_val, MAX_TOKEN_NAME_LENGTH * 3);
    char obj_single_ctor_val[MAX_TOKEN_NAME_LENGTH * 3]; FLOOD_ZEROS(obj_single_ctor_val, MAX_TOKEN_NAME_LENGTH * 3);
    sprintf(obj_arr_ctor_val, "DC__%s_UCTOR_ARR", objects[total_objects-1].class_name);
    sprintf(obj_single_ctor_val, "DC_%s_", objects[total_objects-1].class_name);
    if(is_at_substring(classes[total_classes].member_values[len],obj_arr_ctor_val)
      || is_at_substring(classes[total_classes].member_values[len],obj_single_ctor_val)){
      strcpy(classes[total_classes].member_value_user_ctor[len], classes[total_classes].member_values[len]);
      FLOOD_ZEROS(classes[total_classes].member_values[len], MAX_DEFAULT_VALUE_LENGTH);
      if(!is_obj_ptr) sprintf(classes[total_classes].member_values[len], "{0}");
      else            sprintf(classes[total_classes].member_values[len], "0");
    } else {
      classes[total_classes].member_value_user_ctor[len][0] = 0;
      classes[total_classes].member_value_user_ctor[len][1] = '\0';
    }

    while(IS_WHITESPACE(*member_end) || *member_end == '=') // move ptr to end of member name to copy
      --member_end, ++distance_back; 
    --distance_back;                                        // [start, end) so move end ptr right after name
  } else {                                                  // no initialized value: set to 0
    classes[total_classes].member_values[len][0] = 0;
    classes[total_classes].member_values[len][1] = '\0';
    distance_back = 0;
  }
  return distance_back;
}

// accounts for whether member is itself a class object, & adds it to the 'objects' struct array if so
void register_member_class_objects(char *member_end) {
  // move 'member_end' back from line's end to line's front (to get member data type)
  while(*member_end != '\0' && *member_end != '\n') --member_end;
  while(IS_WHITESPACE(*member_end)) ++member_end;
  int latest_member = classes[total_classes].total_members;

  // check as to whether member is tagged with the "immortal" keyword
  // objects tagged as "immortal" before they're type never invoke user-defined class destructors
  bool is_immortal = false;
  if(*IMMORTALITY) {
    char *check_mortality = member_end, keyword[MAX_TOKEN_NAME_LENGTH];
    FLOOD_ZEROS(keyword, MAX_TOKEN_NAME_LENGTH);
    char *key_ptr = keyword;
    while(VARCHAR(*check_mortality)) *key_ptr++ = *check_mortality++;
    *key_ptr = '\0';
    is_immortal = (strcmp(keyword, "immortal") == 0);
    if(is_immortal) { // skip over "immortal" keyword & up to the start of the member's typename
      member_end = check_mortality;
      while(IS_WHITESPACE(*member_end)) ++member_end;
    }
  }

  // store member data-type/name & pointer/array status
  char member_type[MAX_TOKEN_NAME_LENGTH], member_name[MAX_TOKEN_NAME_LENGTH]; 
  int idx = 0;
  bool is_class_pointer = false, is_class_array = false;
  FLOOD_ZEROS(member_type, MAX_TOKEN_NAME_LENGTH); FLOOD_ZEROS(member_name, MAX_TOKEN_NAME_LENGTH);

  // find the member's data type & determine if type is a class name (thus member = class object)
  while(VARCHAR(*member_end)) member_type[idx++] = *member_end++; 
  member_type[idx] = '\0';
  for(int i = 0; i < total_classes; ++i) {
    if(strcmp(classes[i].class_name, member_type) == 0) {

      // get member/object name & pointer/array status
      while(IS_WHITESPACE(*member_end)) ++member_end;
      if(*member_end == '*') is_class_pointer = true, member_end++;   // check if member is class pointer
      idx = 0;
      while(VARCHAR(*member_end)) member_name[idx++] = *member_end++; // copy member/class-object name
      if(*member_end == '[') is_class_array = true;                   // check if member is class array
      member_name[idx] = '\0';

      // record whether member class object has a alloc'd member to destroy/free
      if(classes[i].class_has_alloc) classes[total_classes].class_has_alloc = true;

      // record member class object's class name
      strcpy(classes[total_classes].member_object_class_name[latest_member], member_type);
      
      // register class object member as one of its class' objects
      strcpy(objects[total_objects].object_name, member_name);
      strcpy(objects[total_objects].class_name, member_type);
      objects[total_objects].is_class_pointer = is_class_pointer;
      objects[total_objects].is_class_array = is_class_array;
      // register object & member mortality
      classes[total_classes].member_is_immortal[latest_member] = is_immortal;
      objects[total_objects].is_immortal = is_immortal;
      total_objects++;
      return;
    }
  }
  // if not a class, class name is 0
  classes[total_classes].member_object_class_name[latest_member][0] = 0;
  classes[total_classes].member_object_class_name[latest_member][1] = '\0';
  classes[total_classes].member_is_immortal[latest_member] = false; // non-obj's not dtor'd so not affected by "immortal"
}

// check if sizeof() arg is the member just created (ie *node = malloc(sizeof(node));) and if so, 
// prepend sizeof arg with "this" (ie sizeof(this.node)) for global initializer fcn
void check_for_alloc_sizeof_arg() {
  int len = classes[total_classes].total_members;
  if(classes[total_classes].member_values[len][0] == 0 || !classes[total_classes].member_value_is_alloc[len]) return;
  char *p=classes[total_classes].member_values[len], *name=classes[total_classes].member_names[len], *size, *prep, *q;
  char member_name[MAX_DEFAULT_VALUE_LENGTH], prepended_sizeof_arg[260];
  FLOOD_ZEROS(member_name, MAX_DEFAULT_VALUE_LENGTH); FLOOD_ZEROS(prepended_sizeof_arg, 260);
  size = member_name, prep = prepended_sizeof_arg;
  while(*p != '\0' && strlen(p) > 7) {
    if(is_at_substring(p, "sizeof(")) {
      p += 7;                                          // skip past sizeof
      while(*p != '\0' && !VARCHAR(*p)) ++p;           // skip past potential '*'
      char *startOfSizeofArg = p;
      while(*p != '\0' && VARCHAR(*p)) *size++ = *p++; // copy name
      *size = '\0';
      if(strcmp(member_name, name) == 0) { // check if sizeof() arg = newest member (ie *node = malloc(sizeof(node));)
        q = classes[total_classes].member_values[len];
        while(*q != '\0' && q != startOfSizeofArg) *prep++ = *q++; // copy up to sizeof arg
        sprintf(prep, "this.");                                    // prepend size of arg with "this"
        prep += strlen(prep);
        while(*q != '\0') *prep++ = *q++;                          // copy rest of sizeof arg
        *prep = '\0';
        FLOOD_ZEROS(classes[total_classes].member_values[len], MAX_DEFAULT_VALUE_LENGTH);
        strcpy(classes[total_classes].member_values[len], prepended_sizeof_arg);
        break;
      }
    }
    ++p;
  }
}

// check for & store member in class - returns 0 = no member, 1 = member, 2 = member initialized with values
int get_class_member(char *end, bool is_fcn_ptr) {
  char *member_start, *member_end = end;
  while(IS_WHITESPACE(*member_end)) --member_end;
  if(*member_end == ';' || is_fcn_ptr) {                             // is member
    register_member_class_objects(member_end);                       // account for whether member is itself a class obj
    if(is_fcn_ptr) 
      while(*member_end != '\0' && *member_end != ';') ++member_end; // to check for initialization value
    int len = classes[total_classes].total_members;                  // new member index
    member_end -= get_initialized_member_value(member_end);          // get initialized value (0 by default)
    
    if(*(member_end - 1) == ']') {                                   // is array member
      while(*member_end != '[') --member_end; 
      classes[total_classes].member_is_array[len] = true;
    } else classes[total_classes].member_is_array[len] = false;

    // if user using a ptr obj thats ctor'd & alloc'd, format "className *objName = ctor() = alloc()"
    // BUT member end currently directly prior "alloc()"'s equal sign right now so register's ctor's
    char *check_for_obj_ptr_alloc = member_end;
    if(*check_for_obj_ptr_alloc == '=') --check_for_obj_ptr_alloc; // if no spaces padding '=' "member_end" will be @ '='
    while(no_overlap(*check_for_obj_ptr_alloc, "\n=")) --check_for_obj_ptr_alloc;
    if(*check_for_obj_ptr_alloc == '=') {   // CONTAINED OBJECT POINTER BEING CTOR'D & ALLOC'D !!!
      while(!VARCHAR(*check_for_obj_ptr_alloc)) --check_for_obj_ptr_alloc;
      ++check_for_obj_ptr_alloc;
      member_end = check_for_obj_ptr_alloc; // move end of member object's name right after last '='
    }
    
    member_start = member_end;                                       // [member_start, member_end)
    while(!IS_WHITESPACE(*(member_start - 1))) --member_start;

    char *find_asterisk = member_start;                              // determine if pointer member
    while(no_overlap(*find_asterisk, "*\n;") && !VARCHAR(*find_asterisk)) --find_asterisk;
    classes[total_classes].member_is_pointer[len] = (*find_asterisk == '*');

    int i = 0;                                                       // copy member to classes struct
    while(member_start != member_end) {
      if(VARCHAR(*member_start)) classes[total_classes].member_names[len][i++] = *member_start;
      ++member_start;
    }
    classes[total_classes].member_names[len][i] = '\0';
    check_for_alloc_sizeof_arg(); // prepend alloc sizeof() arg w/ "this." if arg = newest member
    classes[total_classes].total_members += 1;
    return 1 + (classes[total_classes].member_values[len][0] != 0);
  }
  return 0;
}

// adds a word from method into 'method_words[][]'
void add_method_word(char method_words[][MAX_TOKEN_NAME_LENGTH], int *word_size, char *word_start, char *word_end) {
  char *p = word_start, method_word[MAX_TOKEN_NAME_LENGTH];
  FLOOD_ZEROS(method_word, MAX_TOKEN_NAME_LENGTH);
  int i = 0, j;
  while(p - 1 != word_end) method_word[i++] = *p++;
  method_word[i] = '\0';
  for(j = 0; j < *word_size; ++j) if(strcmp(method_words[j], method_word) == 0) break;
  if(j == *word_size) {
    strcpy(method_words[*word_size], method_word);
    *word_size += 1;
  }
}

/******************************************************************************
* CONFIRM WHETHER METHOD'S WORD IS A LOCAL CLASS MEMBER
******************************************************************************/

// returns whether var is a redefinition/declaration (false) or member (true)
bool not_local_var_declaration(char *word_start) {
  // if 'basic_c_types' found before ';' or ') {', & isn't an array declaration elt nor array 
  // subscript nor assigned value, var is presumed to be a type redefinition/declaration (ie NOT a member)
  char *scout = word_start, *findParenthesis;
  bool found_close_bracket = false, found_close_brace = false;
  while(*scout != ';') {
    if(*scout == '}') found_close_brace   = true; // not an array declaration elt
    if(*scout == ']') found_close_bracket = true; // not in array subscript
    if(*scout == '[' && !found_close_bracket) return true;
    if(*scout == '{') { // if '{' preceded by 'whitespace' & ')', not a declaration
      findParenthesis = scout - 1;
      while(IS_WHITESPACE(*findParenthesis)) --findParenthesis;
      if(*findParenthesis == ')' || !found_close_brace) return true;
    }
    if(*scout == '=') return true;       // elt is used as an assigned value
    for(int i = 0; i < TOTAL_TYPES; ++i) // check for basic type declaration
      if(is_at_substring(scout, basic_c_types[i]) && !VARCHAR(*(scout - 1))) 
        return false;
    --scout;
  }
  return true;
}

// returns whether method doesn't already have word (if false, var is then NOT a member)
bool not_in_method_words(char method_words[][MAX_TOKEN_NAME_LENGTH], int word_size, char *word) {
  for(int j = 0; j < word_size; ++j) if(strcmp(method_words[j], word) == 0) return false;
  return true;
}

// returns whether word isn't an invocation (true)
// otherwise, being invoked by some external struct or class (false)
bool not_an_external_invocation(char *method_buff_idx) {
  char *p = method_buff_idx;
  while(VARCHAR(*p)) --p;
  while(IS_WHITESPACE(*p)) --p;
  if(*p == '.' || (*p == '>' && *(p - 1) == '-')) return false;
  return true;
}

// given a member's index, splices in 'this->' after member's name
void splice_in_this_arrowPtr(char *method_buff_idx) {
  char members_name[MAX_TOKEN_NAME_LENGTH];
  FLOOD_ZEROS(members_name, MAX_TOKEN_NAME_LENGTH);
  method_buff_idx--;                                  // move ptr back to a char (at '\0' right now)
  while(VARCHAR(*method_buff_idx)) --method_buff_idx; // move ptr back to front of member name
  method_buff_idx++;                                  // increment ptr to right B4 member name
  strcpy(members_name, method_buff_idx);
  sprintf(method_buff_idx, "this->%s", members_name); // splice in 'this->members_name'
}

// finds methods invoked w/in methods of their same class such that they
// should be prepended 'className'_ & take 'this' as their last arg
int parse_local_nested_method(char *end, char *method_buff_idx, char *class_name, char method_words[][MAX_TOKEN_NAME_LENGTH]) {
  int prepended_size = 0;
  if(!VARCHAR(*end) && VARCHAR(*(end + 1))) {
    *method_buff_idx++ = *end++, prepended_size++;                     // move to first letter
    char method_name[MAX_TOKEN_NAME_LENGTH];
    FLOOD_ZEROS(method_name, MAX_TOKEN_NAME_LENGTH);
    int i = 0;
    while(VARCHAR(*end)) method_name[i++] = *end++, prepended_size++;  // copy method name
    if(*end != '(') return 0;                                          // if no method
    method_name[i] = '\0';
    for(int j = 0; j < classes[total_classes].total_methods; ++j)      // find if class has method name
      if(strcmp(classes[total_classes].method_names[j], method_name) == 0) {
        sprintf(method_buff_idx, "DC_%s_%s", class_name, method_name); // prepend method name w/ 'className'_
        method_buff_idx += strlen(method_buff_idx);
        // copy method arguments & prefix any local members w/in w/ 'this->'
        end += prefix_local_members_and_cpy_method_args(end, method_buff_idx, method_words, &prepended_size, ')');
        method_buff_idx += strlen(method_buff_idx);
        (*(end - 1) == '(') ? sprintf(method_buff_idx, "this") : sprintf(method_buff_idx, ", this");
        return prepended_size;
      }
  }
  return 0;
}

// umbrella fcn evaluating all conditions for a word to be a local class member
bool valid_member(char*word_start,char*memberName,char nextChar,char period,char method_words[][MAX_TOKEN_NAME_LENGTH],int word_size) {
  return (is_at_substring((word_start+1), memberName) && !VARCHAR(*word_start) // at potential member
    && (!VARCHAR(nextChar) || period == '.')                                   // member = word or struct
    && not_local_var_declaration(word_start+1)                                 // not redeclared
    && not_in_method_words(method_words, word_size, memberName)                // not redefined
    && not_an_external_invocation(word_start+1));                              // not external invocation
}

/******************************************************************************
* PARSE CLASS
******************************************************************************/

// parses class instance
int parse_class(char *class_instance, char *NEW_FILE, int *j) {
  #define skip_over_blank_lines(first_newline) \
    blank_line_size = remove_blank_lines(first_newline); \
    if(blank_line_size > 0) { \
      class_size += blank_line_size; \
      end += blank_line_size; \
      start_of_line = end; \
    }

  // method_buff stores functions in class to be spliced out
  char method_buff[MAX_METHOD_BYTES_PER_CLASS], *method_buff_idx;
  FLOOD_ZEROS(method_buff, MAX_METHOD_BYTES_PER_CLASS);
  
  // struct_buff stores non-method elts from class to be put into struct
  char struct_buff[MAX_MEMBER_BYTES_PER_CLASS], class_name[MAX_TOKEN_NAME_LENGTH], *struct_buff_idx;
  FLOOD_ZEROS(struct_buff, MAX_MEMBER_BYTES_PER_CLASS);
  FLOOD_ZEROS(class_name, MAX_TOKEN_NAME_LENGTH);
  get_class_name(class_instance, class_name);
  sprintf(struct_buff, "typedef struct DC_%s {\n\tint *DC_DTR;", class_name);
  struct_buff_idx = &struct_buff[strlen(struct_buff)]; // points to '\0'
  method_buff_idx = method_buff;

  // store dflt class info in the global "classes" struct
  initialize_new_class_default_properties(class_name);

  // dummy ctor detection variables
  char dummy_ctor_invoking_obj_chain[MAX_TOKEN_NAME_LENGTH * 2];
  bool dummy_ctor_ptr_invoked = false, dummy_ctor_invoker_redefined = false;
  int dummy_ctor_len;

  // # of class or comment chars
  int class_size = 0, class_comment_size, blank_line_size;

  // class scope btwn 'start' & 'end'
  char *start = class_instance, *end, *start_of_line;
  int in_class_scope = 1, in_method_scope;
  bool in_a_string, in_a_char, in_token_scope;
  while(*start++ != '{') ++class_size;
  end = start;

  // copy members to struct_buff & methods to method_buff
  while(*end != '\0' && in_class_scope > 0) {
    // confirm in class' scope
    if(*end == '{') in_class_scope++;
    else if(*end == '}') in_class_scope--;
    if(in_class_scope < 0) break;

    // skip over struct/union members - the struct's variables are considered 
    // within the struct's scope and thus NOT members of the outer class
    // NOTE: UNIONS & STRUCTS HANDLED IDENTICALLY!
    if((is_at_substring(end, "struct") || is_at_substring(end, "union")) && is_struct_definition(end)) {
      while(*end != '\0' && *(end - 1) != '{') *struct_buff_idx++ = *end++, ++class_size;
      int in_struct_scope = 1;
      while(*end != '\0' && in_struct_scope > 0) {
        if(*end == '{') in_struct_scope++;
        else if(*end == '}') in_struct_scope--;
        if(*end == ';') { // store struct members to use in CTOR brace initialization
          #define init_struct_members(attrib) ({\
            classes[total_classes].attrib[classes[total_classes].total_members][0] = 0; \
            classes[total_classes].attrib[classes[total_classes].total_members][1] = '\0'; \
          })
          init_struct_members(member_names); 
          init_struct_members(member_values);
          #undef init_struct_members
          classes[total_classes].total_members += 1;
        }
        *struct_buff_idx++ = *end++, ++class_size;
      }
    }

    // check for newline - methods & members
    if(*end == '\n') {
      start_of_line = end;
      if(get_class_member(end, false) == 2) { // check for member & rmv any init vals from struct
        *struct_buff_idx-- = '\0';
        while(*struct_buff_idx != '\0' && *struct_buff_idx != '\n' && *struct_buff_idx != '=') *struct_buff_idx-- = '\0';
        *struct_buff_idx++ = ';';

        char *check_for_obj_ptr_alloc = struct_buff_idx - 1;
        while(no_overlap(*check_for_obj_ptr_alloc, "\n=")) --check_for_obj_ptr_alloc;
        // only ptr objs both ctor'd and alloc'd have a "className *objName = ctor() = alloc();" format
        if(*check_for_obj_ptr_alloc == '=') { 
          // directly copy the obj ctor into the class object "member_value_user_ctor" attribute

          char *obj_ptr_alloc = check_for_obj_ptr_alloc + 1;
          int obj_ptr_idx = 0, current_member = classes[total_classes].total_members - 1;
          while(IS_WHITESPACE(*obj_ptr_alloc)) ++obj_ptr_alloc;

          // confirm at an object pointer being constructed and alloc'd (only instance where a valid double assignment
          // could occur, and even then it's generated by declass.c not the user)
          if(!is_at_substring(obj_ptr_alloc, "DC_")) throw_potential_invalid_double_dflt_assignment(__func__, __LINE__, class_name);

          FLOOD_ZEROS(classes[total_classes].member_value_user_ctor[current_member], MAX_TOKEN_NAME_LENGTH);
          while(obj_ptr_alloc != struct_buff_idx) // copy ctor into class of "classes" struct array's ctor value
            classes[total_classes].member_value_user_ctor[current_member][obj_ptr_idx++] = *obj_ptr_alloc++;
          classes[total_classes].member_value_user_ctor[current_member][obj_ptr_idx-1] = '\0';

          while(struct_buff_idx != check_for_obj_ptr_alloc) *struct_buff_idx-- = '\0'; // erase the ctor from struct buff
          *struct_buff_idx++ = ';';
        }
      }
      skip_over_blank_lines(end); // don't copy any blank lines to struct_buff
    }

    // check for function method
    if(*end == '(') { 
      // check for function assignment -- especially for 'malloc'/'calloc'
      char *findEq = end;
      while(no_overlap(*findEq, "\n=;")) --findEq; // move to either beginning of line or assignment
      if(*findEq == '=') {                         // if function is being assigned
        while(*end != '\0' && *end != '\n')        // move to the end of the line
          *struct_buff_idx++ = *end++, ++class_size;
        continue;                                  // invoke next loop iteration to trigger "member" detection logic above
      }

      // check for fcn ptr => treated as member
      if(*(end + 1) == '*') { 
        while(*end != '\0' && *end != ')') *struct_buff_idx++ = *end++, ++class_size;
        if(get_class_member(end, true) == 2) { // copy fcn ptr args BUT NOT INITIALIZING VALUE to struct buff
          while(*end != '\0' && *end != '=' && *(end - 1) != '\n') *struct_buff_idx++ = *end++, ++class_size;
          *struct_buff_idx++ = ';';
          *struct_buff_idx++ = '\n';
          while(*end != '\0' && *(end - 1) != '\n') end++, ++class_size;
        } else {                              // copy fcn ptr args to struct buff
          while(*end != '\0' && *(end - 1) != '\n') *struct_buff_idx++ = *end++, ++class_size;
        }
        start_of_line = end - 1;
        skip_over_blank_lines(end - 1);       // don't copy any blank lines to struct_buff
        continue;
      }

      // only 3 cases result in a class member having the structure: "word" "word" '('
      // (1) fcn ptr, but this is handled directly above
      // (2) a method, in which case the last char in the line ought to be either '\n' of '{'
      // (3) a member object being declared with a user-defined constructor, in which case the line ends w/ ';'
      // handle case (3): check for user-defined constructor invocation
      bool found_ctor = false;
      int struct_increment = 0;
      end = check_for_ctor_obj(end, struct_buff_idx, &class_size, &struct_increment, &found_ctor);
      if(found_ctor) {
        struct_buff_idx += struct_increment;
        // move line's end & invoke next loop iteration to trigger "member" detection logic above method detection
        while(*end != '\0' && *end != '\n') *struct_buff_idx++ = *end++, ++class_size; 
        continue;
      }

      // store method variables and keywords to single out local members to make 'this' point to
      char method_words[MAX_WORDS_PER_METHOD][MAX_TOKEN_NAME_LENGTH], *word_start;
      for(int i = 0; i < MAX_WORDS_PER_METHOD; ++i) FLOOD_ZEROS(method_words[i], MAX_TOKEN_NAME_LENGTH);
      int word_size = 0;

      // confirm whether still w/in method scope or w/in a string
      in_method_scope = 1;
      in_a_string = false, in_a_char = false, in_token_scope = true;
      
      // get className-prepended method name
      char prepended_method_name[MAX_TOKEN_NAME_LENGTH];
      FLOOD_ZEROS(prepended_method_name, MAX_TOKEN_NAME_LENGTH);
      bool method_is_ctor = false, method_is_dtor = false;
      get_prepended_method_name(start_of_line, class_name, prepended_method_name, &method_is_ctor, &method_is_dtor);

      // remove method from struct_buff (only members)
      char *wipe_method_line = start_of_line;
      while(wipe_method_line++ != end) *struct_buff_idx-- = '\0';

      // copy method to 'method_buff' & move 'end' forward
      while(IS_WHITESPACE(*start_of_line) && start_of_line != end)  *method_buff_idx++ = *start_of_line++; // copy indent
      if(method_is_dtor) {
        sprintf(method_buff_idx, "void"); method_buff_idx += strlen(method_buff_idx);
      } else {
        while(!IS_WHITESPACE(*start_of_line) && start_of_line != end) *method_buff_idx++ = *start_of_line++; // copy type
      }
      sprintf(method_buff_idx, " %s", prepended_method_name);                              // copy appended method name
      method_buff_idx += strlen(method_buff_idx);                                          // move method_buff_idx to '\0'

      // store method's arg words in 'method_words[][]' to discern from local class member vars
      while(*(end + 1) != '\0' && *end != ')') {
        // Check for class objects in method argument
        int k; 
        bool PLACEHOLDER_ARG_VAL = false;
        for(k = 0; k < total_classes + 1; ++k)
          if(is_at_substring(end, classes[k].class_name) 
            && !VARCHAR(*(end + strlen(classes[k].class_name))) && !VARCHAR(*(end-1))) {
            store_object_info(end, -1, &PLACEHOLDER_ARG_VAL); // register object arg as a class object instance
            // check for unexpected "dummy ctor" invocation
            if(PLACEHOLDER_ARG_VAL) 
              POSSIBLE_DUMMY_CTOR_METHOD_ARG_ERROR_MESSAGE(__func__, __LINE__, k);
            break; 
          }
        // Save method's words in argument
        if(k == total_classes + 1){
          if(!VARCHAR(*end) && *end != ')' && VARCHAR(*(end + 1))) word_start = end + 1;
          else if(VARCHAR(*end) && !VARCHAR(*(end + 1)))
            add_method_word(method_words, &word_size, word_start, end);         // add arg word to 'method_words[][]'
        }
        *method_buff_idx++ = *end++, ++class_size;                              // copy method up to ')'
      }

      // splice in 'this' class ptr as last arg in method
      if(*(end - 1) == '(') sprintf(method_buff_idx, "%s *this", class_name);   // spliced class ptr is single method arg                      
      else                  sprintf(method_buff_idx, ", %s *this", class_name); // spliced class ptr is poly method arg
      method_buff_idx += strlen(method_buff_idx); 
      while(*end != '\0' && *(end - 1) != '{') *method_buff_idx++ = *end++, ++class_size; // copy method up to '{'

      // if method = destructor, splice in destructors for contained objects thus far at dtor's start
      if(method_is_dtor)
        for(int k = 0; k < classes[total_classes].total_members; ++k)
          if(classes[total_classes].member_object_class_name[k][0] != 0
            && !classes[total_classes].member_is_pointer[k]
            && !classes[total_classes].member_is_immortal[k]) { // non-pointer & mortal member objects
            if(classes[total_classes].member_is_array[k]) {
              sprintf(method_buff_idx, "\n\t\tDC__%s_UDTOR_ARR(this->%s);", 
                classes[total_classes].member_object_class_name[k], classes[total_classes].member_names[k]);
            } else if(classes[total_classes].member_is_pointer[k]) {
              sprintf(method_buff_idx, "\n\t\tDC__NOT_%s_(this->%s);", 
                classes[total_classes].member_object_class_name[k], classes[total_classes].member_names[k]);
            } else {
              sprintf(method_buff_idx, "\n\t\tDC__NOT_%s_(&(this->%s));", 
                classes[total_classes].member_object_class_name[k], classes[total_classes].member_names[k]);
            }
            method_buff_idx += strlen(method_buff_idx);
          }

      // copy the rest of the method into 'method_buff'
      in_class_scope++;                                                      // skip first '{'
      while(*end != '\0' && in_method_scope > 0 && in_class_scope > 0) {     // copy method body
        // account for current scope & cpy method
        if(*end == '{') in_method_scope++, in_class_scope++;
        else if(*end == '}') in_method_scope--, in_class_scope--;
        account_for_string_char_scopes(&in_a_string, &in_a_char, &in_token_scope, end);
        if(in_method_scope < 0 || in_class_scope < 0) break;

        // check for class object declaration
        bool dummy_ctor = false;
        for(int k = 0; in_token_scope && k < total_classes + 1; ++k)
          if(is_at_substring(end, classes[k].class_name) 
            && !VARCHAR(*(end-1)) && !VARCHAR(*(end+strlen(classes[k].class_name)))) {
            if(store_object_info(end, 1, &dummy_ctor)) {

              // check if a so-called "dummy ctor" was detected and splice 
              // in the "DC__DUMMY_" class/ctor name's prefix if so
              if(dummy_ctor) {
                dummy_ctor_ptr_invoked = dummy_ctor_invoker_redefined = false;
                FLOOD_ZEROS(dummy_ctor_invoking_obj_chain, MAX_TOKEN_NAME_LENGTH * 2);
                // flag the object to be destroyed again after being assigned to a dummy ctor (being that they're a new obj instance)
                get_dummy_ctor_invoking_object_chain(end, dummy_ctor_invoking_obj_chain, k, &dummy_ctor_ptr_invoked, &dummy_ctor_invoker_redefined);
                dummy_ctor_len = prefix_dummy_ctor_with_DC__DUMMY_(method_buff_idx, end);
                method_buff_idx += strlen(method_buff_idx), end += dummy_ctor_len, class_size += dummy_ctor_len;
                // As per Documentation's "DECLASS.C CTORS & DTORS" > "FORMATTING" > "(2)", dummy ctors only 
                // return single obj instances, thus "obj arrays" are presumed to be assigning one of their
                // individual object "cells" w/in.
                add_object_dtor(end, true, dummy_ctor_invoker_redefined); 
                if(dummy_ctor_invoker_redefined) { // Don't add for new declarations
                  if(dummy_ctor_ptr_invoked)
                    sprintf(method_buff_idx, ";if(%s){%s->DC_DTR=&DC__NDTR;}", dummy_ctor_invoking_obj_chain, dummy_ctor_invoking_obj_chain);
                  else sprintf(method_buff_idx, ";%s.DC_DTR=&DC__NDTR", dummy_ctor_invoking_obj_chain);
                  method_buff_idx += strlen(method_buff_idx);
                }
                break;
              }

              // check if object is assigned a value upon declaration (if so, no default values need be added)
              char *already_assigned = end;
              while(*already_assigned != '\0' && no_overlap(*already_assigned, "\n;,=")) ++already_assigned;
              if(*already_assigned == '=' && !objects[total_objects-1].is_alloced_class_pointer) {
                add_object_dtor(end, true, false);
                break;
              }

              // determine if object is invoking it's user-defined constructor
              char *user_ctor_finder = end;
              char user_ctor[1000]; FLOOD_ZEROS(user_ctor, 1000);
              bool is_fcn_returning_obj = false;
              bool user_ctor_invoked=get_user_ctor(user_ctor_finder,user_ctor,classes[k].class_name,&is_fcn_returning_obj);
              // don't splice in any constructors if "object" is actually a fcn returning an object
              if(is_fcn_returning_obj) break;
              // implement macro ctor
              while(*end != '\0' && *(end-1) != ';') *method_buff_idx++ = *end++, ++class_size;
              // if an obj ptr allocating memory, confirm obj != NULL prior to passing to ctors & dflt-val assignment
              objects[total_objects-1].is_dangling_ctord_ptr = false; // ID dangling ptrs (no ctor/'='/alloc) to assign "objPtr=NULL" in "add_object_dtor()"
              if(objects[total_objects-1].is_alloced_class_pointer) {
                sprintf(method_buff_idx, " if(%s){", objects[total_objects-1].object_name), method_buff_idx += strlen(method_buff_idx);
              // if obj a ctor'd ptr but not alloc'd, still assign dangling ptr to NULL prior any CTOR's
              } else if(objects[total_objects-1].is_class_pointer) {
                sprintf(method_buff_idx, " %s=NULL;", objects[total_objects-1].object_name), method_buff_idx += strlen(method_buff_idx);
                objects[total_objects-1].is_dangling_ctord_ptr = true;
              }
              // only add dflt vals for non-ptrs or memory-allocated pointers
              if(!objects[total_objects-1].is_class_pointer || objects[total_objects-1].is_alloced_class_pointer) {
                if(objects[total_objects-1].is_class_array)        // object = array, use array macro init
                  sprintf(method_buff_idx, " DC__%s_ARR(%s);", 
                    classes[k].class_name, objects[total_objects-1].object_name);
                else if(objects[total_objects-1].is_class_pointer) // object != array & is ptr, so use single-object macro init
                  sprintf(method_buff_idx, " DC__%s_CTOR((*%s));%s->DC_DTR=&DC__NDTR;", 
                    classes[k].class_name, objects[total_objects-1].object_name, objects[total_objects-1].object_name);
                else                                               // object != array, so use single-object macro init
                  sprintf(method_buff_idx, " DC__%s_CTOR(%s);", 
                    classes[k].class_name, objects[total_objects-1].object_name);
                method_buff_idx += strlen(method_buff_idx);
              }
              // add user-defined ctor invocation w/ initialization values (if present)
              if(user_ctor_invoked) { 
                sprintf(method_buff_idx, " %s", user_ctor); 
                method_buff_idx += strlen(method_buff_idx); 
              }
              // if an obj ptr allocing memory, close the "if != null" braced-condition
              if(objects[total_objects-1].is_alloced_class_pointer) *method_buff_idx++ = '}';
              add_object_dtor(end, false, false);
            }
            break;
          }

        // either add method word to 'method_words[][]' or prepend 'this->' to member
        if(in_token_scope) {
          if(!VARCHAR(*(end-1)) && (*(end-1) != '\'' || (*(end-2) == '\\' && *(end-3) != '\\')) && VARCHAR(*end)) word_start = end; // beginning of word
          if(VARCHAR(*end) && !VARCHAR(*(end + 1))) { // end of word - member or var
            // check if a member of the current/local class
            int i = 0;
            for(; i < classes[total_classes].total_members; ++i) {
              if(classes[total_classes].member_names[i][0] == 0) continue; // struct member -- disregard
              char *endOfMember = word_start + strlen(classes[total_classes].member_names[i]);
              char nextChar = *endOfMember, period = *(endOfMember + 1);   // either a word or class' struct member invoked
              // word is a member of the local class -- prefix 'this->'
              if(valid_member(word_start - 1, classes[total_classes].member_names[i], 
                nextChar, period, method_words, word_size)) {

                splice_in_this_arrowPtr(method_buff_idx);
                method_buff_idx += strlen(method_buff_idx);
                break;
              }
            }
            // not a member - thus add arg word to 'method_words[][]'
            if(i == classes[total_classes].total_members) add_method_word(method_words, &word_size, word_start, end); 
          }
        }

        // prepend methods invoked within methods with the appropriate 'className'_
        int method_buff_idx_position = strlen(method_buff) + 1;
        int nested_method_size = (in_token_scope)
          ? parse_method_invocation(end, method_buff_idx, &method_buff_idx_position, true, method_words)
          : 0;
        end += nested_method_size, class_size += nested_method_size;

        // check whether nested method or not
        if(nested_method_size > 0) {
          method_buff_idx += strlen(method_buff_idx);
        } else {

          // check whether nested method, but for local class (ie NOT for another object)
          int local_nested_method_size = (in_token_scope) 
            ? parse_local_nested_method(end, method_buff_idx, class_name, method_words) 
            : 0;
          if(local_nested_method_size > 0) {
            end += local_nested_method_size, class_size += local_nested_method_size;
            method_buff_idx += strlen(method_buff_idx);
          }
          *method_buff_idx++ = *end++, ++class_size;
        }
      }
      if(method_is_ctor) { // return object at the end of user-defined ctors for default assignment
        sprintf(method_buff_idx - 1, "\treturn *this;\n\t}");
        method_buff_idx += strlen(method_buff_idx);
      }
      start_of_line = end; // start next line after method
    } 
    *struct_buff_idx++ = *end++;
    ++class_size;
  }

  // provide a default destructor/constructor if left undefined by user
  char default_ctor_dtor[MAX_METHOD_BYTES_PER_CLASS];
  FLOOD_ZEROS(default_ctor_dtor, MAX_METHOD_BYTES_PER_CLASS);
  char *default_cd = default_ctor_dtor;

  // add a default destructor to invoke any dtors of contained objects if user left dtor undefined
  if(!classes[total_classes].class_has_dtor) {
    sprintf(default_cd, "\nvoid DC__NOT_%s_(%s *this) {", class_name, class_name);
    default_cd += strlen(default_cd);
    for(int k = 0; k < classes[total_classes].total_members; ++k) {
      // if found a mortal & non-pointer class member object
      if(!classes[total_classes].member_is_immortal[k] 
        && classes[total_classes].member_object_class_name[k][0] != 0
        && !classes[total_classes].member_is_pointer[k]) {
        if(classes[total_classes].member_is_array[k])        // dtor contained object array
          sprintf(default_cd, "\n\tDC__%s_UDTOR_ARR(this->%s);", 
            classes[total_classes].member_object_class_name[k], classes[total_classes].member_names[k]);
        else if(classes[total_classes].member_is_pointer[k]) // dtor contained single object
          sprintf(default_cd, "\n\tDC__NOT_%s_(this->%s);", 
            classes[total_classes].member_object_class_name[k], classes[total_classes].member_names[k]);
        else                                                 // dtor contained single object
          sprintf(default_cd, "\n\tDC__NOT_%s_(&(this->%s));", 
            classes[total_classes].member_object_class_name[k], classes[total_classes].member_names[k]);
        default_cd += strlen(default_cd);
      }
    }
    sprintf(default_cd, "\n}");
    default_cd += strlen(default_cd);
    classes[total_classes].class_has_dtor = true;
  }

  // add a default constructor to always allow "()" invocation as well as w/o "()" if user left ctor undefined
  if(!classes[total_classes].class_has_ctor) {
    sprintf(default_cd, "\n%s DC_%s_(%s*this){return*this;}", 
      class_name, class_name, class_name);
    default_cd += strlen(default_cd);
    classes[total_classes].class_has_ctor = true;
  }

  // add a so-called "dummy" object-less constructor(s) to return an object:
  // used as an assignment value, denoted as "className objName = className(args);" 
  // with the "dummy ctor" in this example being on the RHS
  mk_dummy_ctor_macros(default_cd, class_name);

  // clean-up formatting of struct & method buffers
  sprintf(struct_buff_idx, " %s;", class_name);
  *method_buff_idx = '\0';

  ++end, ++class_size; // skip '};'
  if(*(end - 1) == ';') ++end, ++class_size;

  // copy the constructor macros, class-converted-to-struct, & spliced-out methods to 'NEW_FILE'
  char macro_ctor_comment[MAX_TOKEN_NAME_LENGTH * 2], macro_dtor_comment[MAX_TOKEN_NAME_LENGTH * 2];
  char struct_comment[MAX_TOKEN_NAME_LENGTH * 2], method_comment[MAX_TOKEN_NAME_LENGTH * 2], dflt_comment[MAX_TOKEN_NAME_LENGTH * 2];
  FLOOD_ZEROS(macro_ctor_comment, MAX_TOKEN_NAME_LENGTH * 2); FLOOD_ZEROS(macro_dtor_comment, MAX_TOKEN_NAME_LENGTH * 2);
  FLOOD_ZEROS(struct_comment, MAX_TOKEN_NAME_LENGTH * 2); FLOOD_ZEROS(method_comment, MAX_TOKEN_NAME_LENGTH * 2); 
  FLOOD_ZEROS(dflt_comment, MAX_TOKEN_NAME_LENGTH * 2);
  sprintf(macro_ctor_comment, "/* \"%s\" CLASS DEFAULT VALUE MACRO CONSTRUCTORS: */\n", class_name);
  sprintf(macro_dtor_comment, "\n/* \"%s\" CLASS OBJECT ARRAY MACRO DESTRUCTOR: */\n", class_name);
  sprintf(struct_comment, "\n\n/* \"%s\" CLASS CONVERTED TO STRUCT: */\n", class_name);
  sprintf(method_comment, "\n\n/* \"%s\" CLASS METHODS SPLICED OUT: */", class_name);
  sprintf(dflt_comment, "\n\n/* DEFAULT PROVIDED \"%s\" CLASS CONSTRUCTOR/DESTRUCTOR: */", class_name);

  // make a global class object with default values to initialize client's unassigned class objects with
  char initial_values_brace[1000];
  FLOOD_ZEROS(initial_values_brace, 1000);
  mk_initialization_brace(initial_values_brace, total_classes);
  char class_global_initializer[6000];
  FLOOD_ZEROS(class_global_initializer, 6000);
  mk_class_global_initializer(class_global_initializer, class_name, initial_values_brace);

  // make macro ctors to assign any objects of this class its default values, as well as
  // initialize any contained class object members too - both for single & array instances of objects
  char ctor_macros[6000];
  FLOOD_ZEROS(ctor_macros, 6000); mk_ctor_macros(ctor_macros, class_name); 

  // make macro dtor to invoke user-defined (or default if undefined by user) dtor across an array of objects
  char dtor_array_macro[6000];
  FLOOD_ZEROS(dtor_array_macro, 6000); 
  mk_dtor_array_macro(dtor_array_macro, class_name); 

  // struct before methods to use class/struct type for method's 'this' ptr args
  if(strlen(struct_buff) > 0) {
    APPEND_STR_TO_NEW_FILE("/******************************** CLASS START ********************************/\n");
    APPEND_STR_TO_NEW_FILE(macro_ctor_comment); APPEND_STR_TO_NEW_FILE(ctor_macros);
    APPEND_STR_TO_NEW_FILE(macro_dtor_comment); APPEND_STR_TO_NEW_FILE(dtor_array_macro);
    APPEND_STR_TO_NEW_FILE(struct_comment); APPEND_STR_TO_NEW_FILE(struct_buff);
    APPEND_STR_TO_NEW_FILE(class_global_initializer);
    APPEND_STR_TO_NEW_FILE(dflt_comment); APPEND_STR_TO_NEW_FILE(default_ctor_dtor);
    if(strlen(method_buff)>0)APPEND_STR_TO_NEW_FILE(method_comment);APPEND_STR_TO_NEW_FILE(method_buff);
    APPEND_STR_TO_NEW_FILE("\n/********************************* CLASS END *********************************/");
  }

  total_classes++;
  return class_size;
  #undef skip_over_blank_lines
}



/******************************************************************************
*******************************************************************************
*  ___                 ____        /|======================================|\
* |`  \              /`    `\      ||                                    O ||
* | |\ \            /        \     || //==\\   //==\\   |\       //^\\  Oo ||
* | | ) ) /|       (      _   \    || ||  |/  ||    ||  ||   _  |/===\| o  ||
* | |/ / |`|        \    ` )   )   || ||       \\==//   ||  ||  ||   || |\ ||
* |   /  | |         \    /   /    || \\     //=====\\  \===//  |/   \| // ||
* |  /   | |__        \__/   /___  ||  \\===//       \=================//  ||
* | |    |  __|        /    /    ) ||                                      ||
* | |    | |          /    (_.  /  ||  O  V  E  R  L  O  A  D  I  N  G  !  ||
* |  \   | |___  /^\ (         /   ||                                      ||
* |___\  \_____\ \_/  \_Pt.2_,/    \|======================================|/
*******************************************************************************
******************************************************************************/



/************************************************************************************
 * ADAPTED/INTEGRATED FROM MY COLA.C PARSER @ https://github.com/jrandleman/Cola   *
 ************************************************************************************
 * cola.c: C OVERLOADED LENGTH ARGUMENTS: PREPROCESS .C FILES OVERLOADING FCNS &   *
 *         MACROS BY THEIR # OF ARGS, & ASSIGNING THEIR FCNS DEFAULT ARG VALUES    *
 ************************************************************************************/

/* STRING, CHAR, AND GLOBAL SCOPES STATUS UPDATING && STRING HELPER FUNCTIONS */
void handle_string_char_brace_scopes(bool*, bool*, int*, char*);
/* COMMENT & MACRO-BODY SKIP/CPY FUNCTIONS */
char *cola_skip_comments(char*, char*);
char *skip_macro_body(char*);
/* "fmacs" (FUNCTION MACRO INSTANCES) STRUCT HELPER FUNCTIONS */
int non_prototype_duplicate_instance_in_fmacs(char*, int);
bool overloaded_name_already_in_fmacs(char*);
void update_all_name_overloaded_status(char*);
void get_fmac_name(char*, char*);
/* MACRO INSTANCE & FUNCTIONLIKENESS EVALUATION FUNCTIONS */
bool is_at_macro_name(char*);
bool macro_is_functionlike(char*);
/* "overload_fmacs" (FUNCTION MACRO OVERLOAD INSTANCES) STRUCT HELPER FUNCTIONS */
bool name_not_in_overload_fmacs(char*);
int overload_fmacs_instance_idx(int);
/* O/P ALIGNMENT SPACE-PADDING FUNCTIONS */
int max_fmacs_name_length();
int max_overload_fmacs_name_length();
int max_dflts_name_length();
void print_space_padding(int, int);
/* MESSAGING FUNCTIONS */
void COLA_in_ASCII();
void terminate_program(char*);
void throw_fatal_error_variadic_overload(char*, char*, int);
void throw_fatal_error_duplicate_overload(char*, int, char*, int);
void throw_fatal_error_undefined_arg_length_invocation(char*, int, char*, int);
void throw_fatal_error_non_functional_invocation_of_overload_or_dflt_name(char*, char*, int);
void throw_fatal_error_invalid_default_arg_value(char*, bool, int);
void throw_fatal_error_ambiguous_overload(char*, int, int, int, int, int);
/* FUNCTION/MACRO ARGUMENT COUNTING FUNCTION */
int count_args(char*, char*, bool, int);
/* FUNCTION-PROTOTYPE CHECKING FUNCTION */
bool is_function_prototype(char*);
/* MAIN ACCOUNTING FUNCTION FOR FUNCTION/MACRO DEFINITIONS IN FILE */
void register_all_global_function_macro_defs(char*);
/* FILTER OVERLOADED FCNS/MACROS FROM "FMACS" INTO "OVERLOAD_FMACS" STRUCTURE */
void filter_overloads_from_FMACS_to_OVERLOAD_FMACS();
/* PREFIX FUNCTION/MACRO OVERLOAD INSTANCES & SPLICE IN DEFAULT FUNCTION VALS */
void prefix_overloaded_and_splice_default_value_instances(char*, char*);
/* DEFAULT VALUE "unique_dflt_fcns" (HAS DFLT FCN INSTANCES) STRUCT HELPER FCNS */
bool name_not_in_unique_dflt_fcns(char*);
int unique_dflt_fcns_instance_idx(int);
int total_dflts_for_ARG_SIZE_fcn_instance_in_UNIQUE_DFLT_FCNS(int, int);
/* DEFAULT VALUE FILTER FCN FROM "ALL_DFLT_FCNS" INTO "UNIQUE_DFLT_FCNS" STRUCT */
void filter_defaulted_fcns_from_ALL_DFLT_FCNS_to_UNIQUE_DFLT_FCNS();
/* DEFAULT VALUE FCNS CONFIRMING DFLT != PRIOR NON-DFLT, IN MACRO, OR AMBIGUOUS */
void confirm_no_ambiguous_overloaded_default_values();
bool ambiguous_overload(int, int, int, int);
void confirm_valid_default_values_position(int, char*, bool);
/* DEFAULT VALUE STORING IN "all_dflt_fcns" & WHITING-OUT FROM ARG LIST FUNCTION */
int store_dflt_value(int, char*);

/******************************************************************************
* MAIN EXECUTION OF "C OVERLOADED LENGTH ARGUMENTS" PARSER
******************************************************************************/

// Main Execution for the "declass.c"-Adapted "cola.c" overloading parser
// Returns Whether or not COLA Overloads Were Found
bool COLA_C_main_execution(bool show_cola_info, char *declass_filename) { 
  char read[MAX_FILESIZE], write[MAX_FILESIZE];
  char filename[MAX_TOKEN_NAME_LENGTH], original_filename_DECLASS[MAX_TOKEN_NAME_LENGTH];
  FLOOD_ZEROS(read, MAX_FILESIZE); FLOOD_ZEROS(write, MAX_FILESIZE);
  FLOOD_ZEROS(filename, MAX_TOKEN_NAME_LENGTH); FLOOD_ZEROS(original_filename_DECLASS, MAX_TOKEN_NAME_LENGTH);
  strcpy(filename, declass_filename);                  // copy original filename
  strcpy(original_filename_DECLASS, declass_filename); // copy original filename
  FSCRAPE(read, filename); // read file

  register_all_global_function_macro_defs(read);    // parse all global fcns/macros
  filter_defaulted_fcns_from_ALL_DFLT_FCNS_to_UNIQUE_DFLT_FCNS(); // filter dflt-arg'd fcns, grouping together any overloads
  filter_overloads_from_FMACS_to_OVERLOAD_FMACS();  // filter fcns/macros, only keeping overloads
  confirm_no_ambiguous_overloaded_default_values(); // confirm no dflt'd fcn & possible overload invocation signature ambiguity

  bool found_overloads = (overload_fmacs_size > 0);
  bool found_defaults  = (unique_dflt_fcns_size > 0);

  if(found_overloads || found_defaults) {
    prefix_overloaded_and_splice_default_value_instances(read, write); // prefix all overloads "DC__<arg#>_"
    COLA_in_ASCII();
  }
  
  // show cola overload/registered global fcn/macro data (if "-l" flag active)
  if(show_cola_info) {
    int max = 0, i = 0, j = 0, k = 0;
    if(found_overloads) {
      max = max_overload_fmacs_name_length();
      printf("\n\033[1m>> \033[4mOVERLOADS\033[0m\033[1m:\033[0m\n");
      for(i = 0; i < overload_fmacs_size; ++i) {
        printf("   %02d) \033[1m\033[4mNAME\033[0m \"%s\"", i + 1, overload_fmacs[i].name);
        print_space_padding(strlen(overload_fmacs[i].name), max);
        printf(" \033[1m\033[4mOVERLOAD ARG LENGTHS\033[0m");      
        for(j = 0; j < overload_fmacs[i].arg_sizes_length; ++j) {
          printf(" %d", overload_fmacs[i].arg_sizes[j]);
          if(j < overload_fmacs[i].arg_sizes_length - 1) printf(",");
        }
        printf("\n");
      }
    }

    if(found_defaults) {
      max = max_dflts_name_length();
      printf("\n\033[1m>> \033[4mDEFAULTS\033[0m\033[1m:\033[0m\n");
      for(i = 0; i < unique_dflt_fcns_size; ++i) {
        printf("   %02d) \033[1m\033[4mNAME\033[0m \"%s\"", i + 1, unique_dflt_fcns[i].name);
        print_space_padding(strlen(unique_dflt_fcns[i].name), max);
        printf(" \033[1m\033[4mDEFAULT FCN ARG VALUES\033[0m \033[1m(\033[4mFcn1\033[0m\033[1m)\033[0m");
        for(j = 0; j < unique_dflt_fcns[i].arg_sizes_length; ++j) {
          for(k = 0; k < unique_dflt_fcns[i].total_dflts[j]; ++k) {
            printf(" %s", unique_dflt_fcns[i].dflt_vals[j][k]);
            if(k < unique_dflt_fcns[i].total_dflts[j] - 1) printf(",");
          }
          if(j < unique_dflt_fcns[i].arg_sizes_length - 1) printf(", \033[1m(\033[4mFcn2\033[0m\033[1m)\033[0m");
        }
        printf("\n");
      }
    }

    max = max_fmacs_name_length();
    printf("\n\033[1m>> \033[4mALL REGISTERED GLOBAL FCNS/MACROS\033[0m\033[1m:\033[0m\n");
    for(i = 0; i < fmacs_size; ++i) {
      printf("   %02d) \033[1m\033[4mNAME\033[0m \"%s\", ", i + 1, fmacs[i].name);
      print_space_padding(strlen(fmacs[i].name), max);
      printf("\033[1m\033[4mIS PROTOTYPE\033[0m %d, \033[1m\033[4mTOTAL ARGS\033[0m %d\n", 
        fmacs[i].is_a_prototype, fmacs[i].args);
    }
  }

  // output results or rename file if no overloads
  if(overload_fmacs_size > 0) {
    printf("\n=================================================================================\n");
    printf("%s ==\033[1mCOLA\033[0m=\033[1mOVERLOAD\033[0m=> ", filename);
    strcpy(&filename[strlen(filename)-10], ".c\0");
    printf("%s", filename);
    if(NO_COMPILE || show_cola_info) printf("\n=================================================================================\n");
    FPUT(write, filename);
  } else { // no need to convert file w/o overloads
    const char *bold_underline = "\033[1m\033[4m";
    if(show_cola_info) printf("\n================================================================================");
    printf("\n\033[1m>> declass.c: cola.c: \033[4mNO\033[0m %sOVERLOADS\033[0m %sOR\033[0m %sDEFAULTS\033[0m %sDETECTED\033[0m\033[1m!\033[0m\n",
      bold_underline, bold_underline, bold_underline, bold_underline);
    strcpy(&original_filename_DECLASS[strlen(original_filename_DECLASS)-10], ".c"); // convert "DECLASS_PRECOLA.c" => "DECLASS.c"
    printf("\033[1m>> Renaming \"\033[0m%s\033[1m\" to \"\033[0m%s\033[1m\"\033[0m\n", filename, original_filename_DECLASS);
    if(rename(filename, original_filename_DECLASS) != 0) {
      fprintf(stderr, 
        "\n\033[1m >> declass.c: cola.c:%03d: \033[33mWARNING\033[0m\033[1m ERROR TRYING TO RENAME \"%s\" TO RMV THE \"_PRECOLA\" POSTFIX!\033[0m", 
        __LINE__, filename);
      fprintf(stderr, "\n                       \033[1mKEEPING \"%s\" AS THE OFFICAL DECLASSIFIED FILENAME!\033[0m\n", filename);
    }
    if(show_cola_info) printf("================================================================================\n");
  }

  return (overload_fmacs_size > 0);
}

/******************************************************************************
* STRING, CHAR, AND GLOBAL SCOPES STATUS UPDATING FUNCTION
******************************************************************************/

// handle incrementing/decrementing status of whether "r" currently in a string, char, or in a braced-scope
void handle_string_char_brace_scopes(bool *in_a_string, bool *in_a_char, int *in_global_scope, char *r) {
  if(!(*in_a_char)   && *r == '"'  && (*(r-1) != '\\' || *(r-2) == '\\')) *in_a_string = !(*in_a_string); // confirm whether in a string or not
  if(!(*in_a_string) && *r == '\'' && (*(r-1) != '\\' || *(r-2) == '\\')) *in_a_char   = !(*in_a_char);   // confirm whether in a char or not
  if(!(*in_a_string) && !(*in_a_char) && *r == '{') ++ *in_global_scope; // update braces scope
  if(!(*in_a_string) && !(*in_a_char) && *r == '}') -- *in_global_scope; // update braces scope
}

/******************************************************************************
* COMMENT & MACRO-BODY SKIP/CPY FUNCTIONS
******************************************************************************/

// if at a comment instance in "*read" skips over them (& copies to "*write" "write" != NULL)
char *cola_skip_comments(char *read, char *write) {
  bool cpy_comments = (write != NULL);            // otherwise only skip over
  if(*read == '/' && *(read + 1) == '/') {        // single-line comment
    if(cpy_comments) *write++ = *read++, *write++ = *read++, *write++ = *read++; else read += 3; // skip/copy initial "//"
    while(*read != '\0' && (*read != '\n' || *(read-1) == '\\')) { // skip/copy comment w/o parsing
      if(cpy_comments) *write++ = *read++; else ++read; 
    }
  } else if(*read == '/' && *(read + 1) == '*') { // block comment
    if(cpy_comments) *write++ = *read++, *write++ = *read++, *write++ = *read++; else read += 3; // skip/copy initial "/*"
    while(*read != '\0' && (*read != '*' || *(read + 1) != '/')) {
      if(cpy_comments) *write++ = *read++; else ++read;            // skip/copy comment w/o parsing
    }
    if(cpy_comments) *write++ = *read++, *write++ = *read++; else read += 2;                     // skip last '*' & '/'
  }
  return read;
}

// invoked at a macro, skips macro body's contents
char *skip_macro_body(char *read) {
  bool in_a_string = false, in_a_char = false;
  int in_global_scope = 0, in_arg_scope = 1;
  ++read; // skip initial '('
  // skip macro arguments to get to body
  while(*read != ')') {
    handle_string_char_brace_scopes(&in_a_string, &in_a_char, &in_global_scope, read);
    if(!in_a_string && !in_a_char && *read == '(')      ++in_arg_scope;
    else if(!in_a_string && !in_a_char && *read == ')') --in_arg_scope;
    if(in_arg_scope <= 0) break;
    ++read;
  }
  // skip macro body
  while(*read != '\0' && (*read != '\n' || *(read-1) == '\\')) ++read;
  return read;
}

/******************************************************************************
* "fmacs" (FUNCTION MACRO INSTANCES) STRUCT HELPER FUNCTIONS
******************************************************************************/

// checks whether exact same fcn/macro already in fmacs (w/ same arg # thus NOT an overload)
int non_prototype_duplicate_instance_in_fmacs(char *name, int total_args) {
  for(int i = 0; i < fmacs_size; ++i) 
    if(strcmp(fmacs[i].name, name) == 0 && fmacs[i].args == total_args) 
      return i;
  return -1;
}

// checks whether same fcn/macro name already in fmacs
bool overloaded_name_already_in_fmacs(char *name) {
  for(int i = 0; i < fmacs_size; ++i) 
    if(strcmp(fmacs[i].name, name) == 0) return true;
  return false;
}

// given a name of a confirmed overloaded fcn/macro instance, updates "overloaded"
// status of all its previously registered overloaded variants
void update_all_name_overloaded_status(char *name) {
  for(int i = 0; i < fmacs_size; ++i) 
    if(strcmp(fmacs[i].name, name) == 0) fmacs[i].overloaded = true;
}

// cpys the macro/fcn name from "read" to "name"
void get_fmac_name(char *read, char *name) {
  while(!VARCHAR(*read)) --read; // move to end of fcn/macro name
  while(VARCHAR(*read))  --read; // move directly prior fcn/macro name
  ++read;                        // move to beginning of fcn/macro name
  while(VARCHAR(*read)) *name++ = *read++; // copy fcn/macro name
  *name = '\0';
}

/******************************************************************************
* MACRO INSTANCE & FUNCTIONLIKENESS EVALUATION FUNCTIONS
******************************************************************************/

// given a ptr to initial '(', returns whether or not "read" name is that of a macro
bool is_at_macro_name(char *read) {
  while(!VARCHAR(*read))      --read; // skip past '('
  while(VARCHAR(*read))       --read; // skip past possible macro name
  while(IS_WHITESPACE(*read)) --read; // skip past whitespace btwn "#define" (OR fcn type) & name
  return (*read == 'e' && *(read-1) == 'n' && *(read-2) == 'i' && *(read-3) == 'f' 
    && *(read-4) == 'e' && *(read-5) == 'd' && *(read-6) == '#');
}

// confirm macro is "functionlike", ie has both a list of args btwn "()" 
// as well as a non-whitespace & non-comment "macro body" (else can't overload)
bool macro_is_functionlike(char *read) { 
  ++read; // skip initial '('
  bool in_a_string = false, in_a_char = false;
  int in_global_scope = 0, in_arg_scope = 1;
  while(in_arg_scope > 0) { // skip past possible macro args (macro body if not function-like)
    handle_string_char_brace_scopes(&in_a_string, &in_a_char, &in_global_scope, read);
    if(!in_a_string && !in_a_char && *read == '(')      ++in_arg_scope;
    else if(!in_a_string && !in_a_char && *read == ')') --in_arg_scope;
    if(in_arg_scope <= 0) break;
    ++read;
  }
  ++read; // move past last closing ')' for possible args (if functionlike, else this closes macro body)
  // confirm macro has non-whitespace & non-comment body (makes it function-like in conjunction w/ having args)
  while(*read != '\0' && (*read != '\n' || *(read-1) == '\\')) { 
    read = cola_skip_comments(read, NULL);
    if(!IS_WHITESPACE(*read)) return true;
    if(*read != '\0' && (*read != '\n' || *(read-1) == '\\')) ++read;
  }
  return false;
}

/******************************************************************************
* "overload_fmacs" (FUNCTION MACRO OVERLOAD INSTANCES) STRUCT HELPER FUNCTIONS
******************************************************************************/

// checks whether same fcn/macro name already in overload_fmacs
bool name_not_in_overload_fmacs(char *name) {
  for(int i = 0; i < overload_fmacs_size; ++i) 
    if(strcmp(overload_fmacs[i].name, name) == 0) return false;
  return true;
}

// returns idx of fcn/macro arg-length overload instance if instance doesn't 
// already have "fmacs[fmacs_idx].args" in "arg_sizes[]", & returns -1 if 
// already found (ie if already stored its name & arg # instance from a prototype)
int overload_fmacs_instance_idx(int fmacs_idx) {
  for(int j = 0, k; j < overload_fmacs_size; ++j) 
    if(strcmp(overload_fmacs[j].name, fmacs[fmacs_idx].name) == 0) {
      // search instance's overloaded arg lengths for "fmacs[fmacs_idx].args"
      for(k = 0; k < overload_fmacs[j].arg_sizes_length; ++k)
        if(fmacs[fmacs_idx].args == overload_fmacs[j].arg_sizes[k]) break;
      // if "fmacs[fmacs_idx].args" not found, return instance idx to 
      // add # of args as a new overload to "overload_fmacs" instance's "arg_sizes[]"
      return (k == overload_fmacs[j].arg_sizes_length) ? j : -1;
    }
  // the following "return -1;" is never triggered -- would indicate "fmacs[fmacs_idx].name" 
  // isn't in the "overload_fmacs" struct, BUT this function is only triggered if the name IS in 
  // "overload_fmacs" as per the "name_not_in_overload_fmacs" function above returning "false"
  return -1; 
}

/******************************************************************************
* O/P ALIGNMENT SPACE-PADDING FUNCTIONS
******************************************************************************/

// get the max fcn/macro name length in "fmacs" struct
int max_fmacs_name_length() {
  int max = 0, length;
  for(int i = 0; i < fmacs_size; ++i)
    if((length = strlen(fmacs[i].name)) > max) max = length;
  return max;
}

// get the max fcn/macro name length in "overload_fmacs" struct
int max_overload_fmacs_name_length() {
  int max = 0, length;
  for(int i = 0; i < fmacs_size; ++i)
    if((length = strlen(overload_fmacs[i].name)) > max) max = length;
  return max;
}

// get the max fcn name length in "unique_dflt_fcns" struct
int max_dflts_name_length() {
  int max = 0, length;
  for(int i = 0; i < unique_dflt_fcns_size; ++i)
    if((length = strlen(unique_dflt_fcns[i].name)) > max) max = length;
  return max;
}

// print spaces to pad btwn the 1st column of names & the 2nd data column to align output
void print_space_padding(int length, int max) {
  for(int i = length; i < max; ++i) printf(" ");
}

/******************************************************************************
* MESSAGING FUNCTIONS
******************************************************************************/

// "COLA" in ASCII
void COLA_in_ASCII() {
  const char *NONE = "\033[0m";                      // clear syntax settings
  const char *BOLD = "\033[1m";                      // bold font
  const char *BLUE = "\033[38;5;21m";                // blue font
  const char *RONW = "\033[48;5;231m\033[38;5;196m"; // Red font ON White back
  const char *WONR = "\033[48;5;196m\033[38;5;231m"; // White font ON Red back
  printf("\n                   /|======================================|\\\n");
  printf("                   ||%s%s        %s          %s        %s          %sO %s||\n", BOLD, RONW, WONR, RONW, WONR, BLUE, NONE);
  printf("                   ||%s%s //==\\\\ %s  //==\\\\  %s |\\     %s  //^\\\\  %sOo %s||\n", BOLD, RONW, WONR, RONW, WONR, BLUE, NONE);
  printf("                   ||%s%s ||  |/ %s ||    || %s ||   _ %s |/===\\| %so  %s||\n", BOLD, RONW, WONR, RONW, WONR, BLUE, NONE);
  printf("                   ||%s%s ||     %s  \\\\==//  %s ||  || %s ||   || |\\ %s||\n", BOLD, RONW, WONR, RONW, WONR, NONE);
  printf("                   ||%s%s \\\\     %s//=====\\\\ %s \\===// %s |/   \\| // %s||\n", BOLD, RONW, WONR, RONW, WONR, NONE);
  printf("                   ||%s%s  \\\\===/%s/       \\=%s========%s========//  %s||\n", BOLD, RONW, WONR, RONW, WONR, NONE);
  printf("                   ||%s%s        %s          %s        %s            %s||\n", BOLD, RONW, WONR, RONW, WONR, NONE);
  printf("                   ||%s%s  O  V  %s  E  R  L %s  O  A  %s  D  E  D ! %s||\n", BOLD, RONW, WONR, RONW, WONR, NONE);
  printf("                   ||%s%s        %s          %s        %s            %s||\n", BOLD, RONW, WONR, RONW, WONR, NONE);
  printf("                   \\|======================================|/\n");
}

// fills "bad_code_buffer" with a snippet of the client's invalid code to show in an error message
void get_invalid_code_snippet(const char *absolute_array_start, const char *scout, char bad_code_buffer[]) {
  int j;
  char *bcb_ptr = bad_code_buffer;
  // move back then copy the invalid code snippet to show in error message
  for(j = 0; scout != absolute_array_start && j < 50; ++j) --scout;
  for(j = 0; scout != '\0' && j < BAD_CODE_BUFFER_LENGTH - 51; ++j) // cpy faulty code snippet
    *bcb_ptr++ = *scout++; 
  *bcb_ptr = '\0';
}

// outputs "message" to stderr, notifies client of cola.c termination, & exits program
void terminate_program(char *message) {
  if(MORTAL_ERRORS) {
    fprintf(stderr, " >> %s\n", message);
    printf(" >> CONTINUE DECLASSIFICATION PROCESS? ENTER 1 FOR YES & 0 FOR NO\n\n>>> ");
    int continueDeclassification;
    scanf("%d", &continueDeclassification);
    if(continueDeclassification == 1)
      printf("\n\n-:- CONTINUING DECLASSIFICATION PROCESS - \033[1mI HOPE YOU KNOW WHAT YOU'RE DOING\033[0m -:-\n\n");
    else {
      printf("\n\n >> Saving declass.c's \"Pre-Cola\" Generated File To Help Client Debug Their Program.\n >> Terminating COLA Processor.\n\n");
      exit(EXIT_SUCCESS); // having been intentionally terminated
    }
  } else {
    fprintf(stderr, " >> %s\n >> Saving declass.c's \"Pre-Cola\" Generated File To Help Client Debug Their Program.\n >> Terminating COLA Processor.\n\n",
      message);
    exit(EXIT_FAILURE);
  }
}

// invoked by finding an overloaded variadic macro/fcn
void throw_fatal_error_variadic_overload(char *name, char *bad_code_buffer, int line_No) {
  fprintf(stderr, "\033[1m\ndeclass.c:cola.c:%03d: \033[31mERROR:\033[0m\033[1m INVALID VARIADIC FCN/MACRO OVERLOAD DETECTED!\n\033[0m", line_No);
  fprintf(stderr, " >> NAME OF VARIADIC FCN/MACRO: \"%s\"\n", name);
  fprintf(stderr, " >> SNIPPET OF THE FILE'S INVALID VARIADIC OVERLOAD:\n");
  fprintf(stderr, "\n\033[1m<CODE_SNIPPET>\n\033[7m%s\033[0m\n\033[1m</CODE_SNIPPET>\033[0m\n\n", bad_code_buffer);
  terminate_program("VARIADIC FCNS/MACROS \033[1mCANNOT\033[0m BE OVERLOADED!");
}

// invoked by detecting overloaded fcn/macro name instance w/ an already existing number of args
void throw_fatal_error_duplicate_overload(char *name, int arg_total, char *bad_code_buffer, int line_No) {
  fprintf(stderr, "\033[1m\ndeclass.c:cola.c:%03d: \033[31mERROR:\033[0m\033[1m DUPLICATE FCN/MACRO ARGUMENT-LENGTH OVERLOAD DETECTED!\n\033[0m", line_No);
  fprintf(stderr, " >> NAME & ARG-LENGTH DUPLICATE INSTANCE OF AN ALREADY EXISTING ARG-LENGTH OVERLOAD: \"%s\", ARG LENGTH: %d.\n", name, arg_total);
  fprintf(stderr, " >> SNIPPET OF THE FILE'S DUPLICATE FCN/MACRO ARGUMENT-LENGTH OVERLOAD:\n");
  fprintf(stderr, "\n\033[1m<CODE_SNIPPET>\n\033[7m%s\033[0m\n\033[1m</CODE_SNIPPET>\033[0m\n\n", bad_code_buffer);
  terminate_program("FUNCTIONS/MACROS W/ THE SAME NUMBER OF ARGUMENTS \033[1mCANNOT\033[0m BE OVERLOADED!");
}

// invoked by detecting fcn/macro name invocation w/ an unaccounted for # of args (global 
// definition of overloaded fcn/macro instance w/ particular arg length not registered by "overload_fmacs")
void throw_fatal_error_undefined_arg_length_invocation(char *name, int arg_total, char *bad_code_buffer, int line_No) {
  fprintf(stderr, "\033[1m\ndeclass.c:cola.c:%03d: \033[31mERROR:\033[0m\033[1m UNDEFINED OVERLOADED ARG LENGTH INVOCATION!\n\033[0m", line_No);
  fprintf(stderr, " >> NO GLOBAL FCN/MACRO DEFINITION OR PROTOTYPE MATCHED THE # OF ARGS FOUND IN THE INVOCATION!\n");
  fprintf(stderr, " >> NAME & ARG-LENGTH OF UNDEFINED OVERLOAD INSTANCE: \"%s\", ARG LENGTH: %d.\n", name, arg_total);
  fprintf(stderr, " >> SNIPPET OF THE FILE'S UNDEFINED OVERLOADED ARG LENGTH INVOCATION:\n");
  fprintf(stderr, "\n\033[1m<CODE_SNIPPET>\n\033[7m%s\033[0m\n\033[1m</CODE_SNIPPET>\033[0m\n\n", bad_code_buffer);
  terminate_program("ALL FCN/MACRO ARG-LENGTH OVERLOADS MUST BE GLOBALLY DEFINED OR PROTOTYPED!");
}

// invoked by detecting fcn/macro name invocation w/o any "args list"
void throw_fatal_error_non_functional_invocation_of_overload_or_dflt_name(char *name, char *bad_code_buffer, int line_No) {
  fprintf(stderr, "\033[1m\ndeclass.c:cola.c:%03d: \033[31mERROR:\033[0m\033[1m REDEFINITION OF OVERLOADED/DEFAULT FCN/MACRO ", line_No);
  fprintf(stderr, "(DETECTED A NON-FUNCTIONAL INVOCATION W/O ARGS & \"()\")!\n\033[0m");
  fprintf(stderr, " >> NAME OF ARG-LESS REDEFINED OVERLOAD/DEFAULT-ARG-VAL INVOCATION INSTANCE: \"%s\".\n", name);
  fprintf(stderr, " >> SNIPPET OF THE FILE'S INVALID INVOCATION INSTANCE:\n");
  fprintf(stderr, "\n\033[1m<CODE_SNIPPET>\n\033[7m%s\033[0m\n\033[1m</CODE_SNIPPET>\033[0m\n\n", bad_code_buffer);
  terminate_program("DEFAULT-ARG-VAL FCNS & OVERLOADED FCN/MACRO NAMES \033[1mCANNOT\033[0m BE REDEFINED TO ANY OTHER VARIABLE/FCN-PTR IN ANY OTHER SCOPE!");
}

// invoked by finding a default value prior to a non-default value in a function's arg list
void throw_fatal_error_invalid_default_arg_value(char *function_name, bool is_a_macro, int line_No) {
  fprintf(stderr, "\033[1m\ndeclass.c:cola.c:%03d: \033[31mERROR:\033[0m\033[1m INVALID DEFAULT ARG ASSIGNMENT!\n\033[0m", line_No);
  if(is_a_macro) {
    fprintf(stderr, " >> DEFAULT VALUE FOR MACRO \"%s\" DETECTED!\n", function_name);
    terminate_program("ONLY FCNS CAN TAKE DEFAULT VALUES, MACROS \033[1mCANNOT\033[0m!");
  }
  fprintf(stderr, " >> DETECTED A DEFAULTED ARG PRECEDING A NON-DEFAULTED ARG IN ARG LIST FOR FCN \"%s\"!\n", function_name);
  terminate_program("ONLY THE LAST ARGS IN A FCN'S ARG LIST CAN BE DEFAULTED, DEFAULTED ARGS \033[1mCANNOT\033[0m PRECEDE NON-DEFAULTED ARGS!");
}

// invoked by finding an ambigious invocation signature btwn 2 overloaded fcn instances wrt the implmentation of default values
void throw_fatal_error_ambiguous_overload(char *function_name, int fcn1_dflts, int fcn1_args, int fcn2_args, int fcn2_dflts, int line_No) {
  fprintf(stderr, "\033[1m\ndeclass.c:cola.c:%03d: \033[31mERROR:\033[0m\033[1m AMBIGUOUS OVERLOAD WITH DEFAULT ARG ASSIGNMENT!\n\033[0m", line_No);
  fprintf(stderr, " >> FCN \"%s\" OVERLOAD & DFLT VALUES MAKE ITS INVOCATION SIGNATURE AMBIGUOUS WRT ITS OTHER OVERLOADS!\n", function_name);
  fprintf(stderr, " >> DATA FOR THE 2 OVERLOADED FCN INSTANCES W/ CONFLICTING AMBIGUOUS INVOCATIONS:\n    OVERLOAD 1:\n\t    ");
  fprintf(stderr, "TOTAL ARGS = %d, TOTAL NON-DFLT ARGS = %d, TOTAL DFLT ARGS = %d", fcn1_args, fcn1_args - fcn1_dflts, fcn1_dflts);
  fprintf(stderr, "\n    OVERLOAD 2:\n\t    TOTAL ARGS = %d, TOTAL NON-DFLT ARGS = %d, TOTAL DFLT ARGS = %d\n", 
    fcn2_args, fcn2_args - fcn2_dflts, fcn2_dflts);
  fprintf(stderr, " >> ANY OVERLOADED FCN W/ DEFAULT ARG VALUES MUST BE DISCERNABLE BY ADHERING TO EITHER OF THE FOLLOWING:\n");
  fprintf(stderr, "    (1) HAVING ITS \"TOTAL NON-DFLT ARGS\" EXCEED THE \"TOTAL ARGS\" OF ALL OTHER OVERLOADED FCN INSTANCES\n");
  fprintf(stderr, "    (2) HAVING ITS \"TOTAL ARGS\" BE LESS THAN THE \"TOTAL NON-DFLT ARGS\" OF ALL OTHER OVERLOADED FCN INSTANCES\n");
  terminate_program("EITHER ALTER \"OVERLOAD 2\"'S \"TOTAL ARGS\" # OR RMV ITS ARG(S) DEFAULT VALUE TO HAVE A UNIQUE INVOCATION\n    SIGNATURE!");
}

/******************************************************************************
* FUNCTION/MACRO ARGUMENT COUNTING FUNCTION
******************************************************************************/

// given a ptr at the opening '(', returns the # of args in a fcn/macro
// & finds default values to be passed to & stored by the "store_dflt_value" fcn
int count_args(char *r, char *function_name, bool is_a_macro, int overload_fmacs_index) {
  char bad_code_buffer[BAD_CODE_BUFFER_LENGTH]; // used to show snippet of client's code at error
  char *absolute_start = r;
  // default-arg-value handling variables
  all_dflt_fcns[all_dflt_fcns_size].total_dflts = 0; // total defaults found so far is 0 (just started parsing)
  strcpy(all_dflt_fcns[all_dflt_fcns_size].fcn_name, function_name);
  FLOOD_ZEROS(all_dflt_fcns[all_dflt_fcns_size].dflt_idxs, MAX_ARGS_PER_FCN);
  int default_value_length; // length of default arg value to white out
  // arg-counting variables 
  bool in_a_string = false, in_a_char = false;
  int in_global_scope = 0; // 0 if in global scope, > 0 if in a fcn or any other braces
  int in_arg_scope = 1, arg_total = 1;
  ++r; // move "r" to 1st arg in fcn & past opening '('
  while(*r != '\0' && in_arg_scope > 0) {
    handle_string_char_brace_scopes(&in_a_string, &in_a_char, &in_global_scope, r);
    if(!in_a_string && !in_a_char) r = cola_skip_comments(r, NULL);
    if(!in_a_string && !in_a_char && *r == '(')      ++in_arg_scope;
    else if(!in_a_string && !in_a_char && *r == ')') --in_arg_scope;
    if(*r == '\0' || in_arg_scope <= 0) break;
    if(!in_a_string && !in_a_char && *r == ',') ++arg_total; // at an arg
    if(!in_a_string && !in_a_char && is_at_substring(r, "...") 
      && overloaded_name_already_in_fmacs(function_name)) {
      FLOOD_ZEROS(bad_code_buffer, BAD_CODE_BUFFER_LENGTH);
      get_invalid_code_snippet(absolute_start, r, bad_code_buffer);
      throw_fatal_error_variadic_overload(function_name, bad_code_buffer, __LINE__); // variadic overload fatal error
    }
    // at a defaulted arg, store its value & whiteout from 'r'
    if(*r == '=' && overload_fmacs_index == -1) { // in 1st round: parsing
      default_value_length = store_dflt_value(arg_total - 1, r);
      while(*r != '\0' && default_value_length > 0) *r++ = ' ', --default_value_length;
      continue;
    }
    ++r;
  }
  --r; // check whether an empty arg list
  while(IS_WHITESPACE(*r)) --r;

  // the 2nd round of splicing in overloaded prefixes can't tell us if at a macro or fcn,
  // thus must compare the overloaded name w/ the number of registered args in order to 
  // determine whether or not currently at a macro
  if(overload_fmacs_index > -1) // in 2nd round of splicing, not parsing overloads
    for(int i = 0; i < overload_fmacs[overload_fmacs_index].arg_sizes_length; ++i)
      if(overload_fmacs[overload_fmacs_index].arg_sizes[i] == arg_total) {
        is_a_macro = overload_fmacs[overload_fmacs_index].is_a_macro[i];
        break;
      }
  if(overload_fmacs_index == -1) { // in 1st round: parsing
    // throw an error if any defaulted args found prior a non-defaulted arg OR in a macro
    confirm_valid_default_values_position(arg_total, function_name, is_a_macro); 
    // register total # of args & increment fcn counter having completed parsing a fcn arg-list instance
    all_dflt_fcns[all_dflt_fcns_size].total_args = arg_total;
    ++ all_dflt_fcns_size;
  }
  return (*r == '(') ? 0 : arg_total;
}

/******************************************************************************
* FUNCTION-PROTOTYPE CHECKING FUNCTION
******************************************************************************/

// given ptr at opening '(' of fcn/macro args list, return whether at a fcn prototype
bool is_function_prototype(char *r) {
  bool in_a_string = false, in_a_char = false, overload;
  int in_global_scope = 0; // 0 if in global scope, > 0 if in a fcn or any other braces
  int in_arg_scope = 1;
  ++r; // move past initial opening '('
  while(*r != '\0' && in_arg_scope > 0) {
    handle_string_char_brace_scopes(&in_a_string, &in_a_char, &in_global_scope, r);
    if(!in_a_string && !in_a_char && *r == '(')      ++in_arg_scope;
    else if(!in_a_string && !in_a_char && *r == ')') --in_arg_scope;
    if(in_arg_scope <= 0) break;
    ++r;
  }
  ++r; // move past closing ")"
  while(IS_WHITESPACE(*r)) ++r; // skip past optional space btwn fcn args list & def
  return (*r == ';'); // prototype
}

/******************************************************************************
* MAIN ACCOUNTING FUNCTION FOR FUNCTION/MACRO DEFINITIONS IN FILE
******************************************************************************/

// register all functions & macros declared globally  along w/ their arg # & overloaded 
// status (fills "fmacs" which then gets filtered into "overload_fmacs" on return to main)
void register_all_global_function_macro_defs(char *read) {
  char *r = read, *scout, function_name[MAX_TOKEN_NAME_LENGTH], bad_code_buffer[BAD_CODE_BUFFER_LENGTH];
  bool in_a_string = false, in_a_char = false, overload, prototype, macro;
  int in_global_scope = 0; // 0 if in global scope, > 0 if in a fcn or any other braces
  int arg_total, duplicate_overload;

  // register all function/macro names in file to detect overloads prior to prefixing invocations
  while(*r != '\0') {
    handle_string_char_brace_scopes(&in_a_string, &in_a_char, &in_global_scope, r);
    if(!in_a_string && !in_a_char)
      r = cola_skip_comments(r, NULL);
    if(!in_a_string && !in_a_char && in_global_scope == 0 && *r == '(') { // potential function/macro
      // check if at a macro, & if so confirm its "functionlike" (can't overload non-functionlike macros)
      macro = is_at_macro_name(r);
      if(macro && !macro_is_functionlike(r)) { 
        r = skip_macro_body(r); 
        continue; 
      } 
      // copy function name
      FLOOD_ZEROS(function_name, MAX_TOKEN_NAME_LENGTH);
      get_fmac_name(r, function_name);
      // get number of args for fcn/macro
      arg_total = count_args(r, function_name, macro, -1);
      // check whether at a function prototype
      prototype = is_function_prototype(r);
      // disregard if function name already exists w/ exact same arg # (ie already registered its prototype)
      if((duplicate_overload = non_prototype_duplicate_instance_in_fmacs(function_name, arg_total)) != -1) { 
        if(!fmacs[duplicate_overload].is_a_prototype) { // throw fatal error if not a prototype
          FLOOD_ZEROS(bad_code_buffer, BAD_CODE_BUFFER_LENGTH);
          get_invalid_code_snippet(read, r, bad_code_buffer);
          throw_fatal_error_duplicate_overload(function_name, arg_total, bad_code_buffer, __LINE__);
        }
        ++r; 
        continue;
      }
      // check if function overload exists w/ same name BUT different # arg
      overload = overloaded_name_already_in_fmacs(function_name);
      if(overload) update_all_name_overloaded_status(function_name);
      // assign unique fcn/macro instance properties
      strcpy(fmacs[fmacs_size].name, function_name);
      fmacs[fmacs_size].args = arg_total;
      fmacs[fmacs_size].overloaded = overload;
      fmacs[fmacs_size].is_a_prototype = prototype;
      fmacs[fmacs_size].is_a_macro = macro;
      ++fmacs_size;
      // check if at a macro to skip over its body
      if(macro) r = skip_macro_body(r);
    }
    if(*r != '\0') ++r;
  }
}

/******************************************************************************
* FILTER OVERLOADED MACROS FROM "FMACS" INTO "OVERLOAD_FMACS" STRUCTURE
******************************************************************************/

// convert "fmacs" struct to "overload_fmacs" struct (disregarding non-overloaded fcns)
void filter_overloads_from_FMACS_to_OVERLOAD_FMACS() {
  int i, overload_idx;
  for(i = 0; i < fmacs_size; ++i) {
    if(fmacs[i].overloaded && name_not_in_overload_fmacs(fmacs[i].name)) {
      // new function/macro name overload instance
      strcpy(overload_fmacs[overload_fmacs_size].name, fmacs[i].name);
      overload_fmacs[overload_fmacs_size].arg_sizes[0] = fmacs[i].args;
      overload_fmacs[overload_fmacs_size].is_a_macro[0] = fmacs[i].is_a_macro;
      overload_fmacs[overload_fmacs_size].arg_sizes_length = 1;
      ++ overload_fmacs_size;
    } else if(fmacs[i].overloaded && (overload_idx = overload_fmacs_instance_idx(i)) != -1) {
      // existing function/macro name overload has a new arg-length overload instance
      overload_fmacs[overload_idx].arg_sizes[overload_fmacs[overload_idx].arg_sizes_length] = fmacs[i].args;
      overload_fmacs[overload_idx].is_a_macro[overload_fmacs[overload_idx].arg_sizes_length] = fmacs[i].is_a_macro;
      ++ overload_fmacs[overload_idx].arg_sizes_length;
    }
  }
}

/******************************************************************************
* PREFIX FUNCTION/MACRO OVERLOAD INSTANCES & SPLICE IN DEFAULT FUNCTION VALS
******************************************************************************/

// prefix all invocation/declaration/definition instances of file's registered
// overloaded fcn/macro names & arg lengths (stored in "overload_fmacs" struct)
// & splice in default values
void prefix_overloaded_and_splice_default_value_instances(char *read, char *write) {
  char *r = read, *w = write, *scout, *next_arg, *arg_end;
  char bad_code_buffer[BAD_CODE_BUFFER_LENGTH];
  bool in_a_string = false, in_a_char = false;
  int i, j, k, arg_total, dflt_args_left, in_arg_scope;
  int ignore_arg = 0; // "global scope" irrelevant here thus now denoted as "ignore_arg"

  // prefix every overloaded instance w/ "COLA__<No_of_Args>_"
  while(*r != '\0') {
    // account for scopes
    handle_string_char_brace_scopes(&in_a_string, &in_a_char, &ignore_arg, r);
    if(!in_a_string && !in_a_char)
      r = cola_skip_comments(r, w), w += strlen(w);
    // -:- PARSE FOR DEFAULTS -:- 
    // check for potential fcn invocation that has default args values
    if(!in_a_string && !in_a_char && VARCHAR(*r) && !VARCHAR(*(r-1))) {
      for(i = 0; i < unique_dflt_fcns_size; ++i) {
        // potential dflt'd fcn arg val instance
        if(is_at_substring(r, unique_dflt_fcns[i].name) && !VARCHAR(*(r+strlen(unique_dflt_fcns[i].name)))) { 
          // get number of args
          scout = r;
          while(VARCHAR(*scout))       ++scout; // skip past name
          while(IS_WHITESPACE(*scout)) ++scout; // skip past optional whitespace btwn name && '('
          
          // dflt'd name invocation w/o any args passed btwn "()" afterwards -- COLA names
          // can NEVER to redefined/reassigned to ANY other variables in ANY other scope
          if(*scout != '(') {
            FLOOD_ZEROS(bad_code_buffer, BAD_CODE_BUFFER_LENGTH);
            get_invalid_code_snippet(read, scout, bad_code_buffer);
            throw_fatal_error_non_functional_invocation_of_overload_or_dflt_name(unique_dflt_fcns[i].name, bad_code_buffer, __LINE__);
          }
          arg_total = count_args(scout, unique_dflt_fcns[i].name, false, i);

          // confirm at a dflt'd instance w/ less args than "dflt-total + nondflt-total" & as least the same args as non-dflt total
          // if same args as nondflt + dflt, no need to splice in anything
          for(j = 0; j < unique_dflt_fcns[i].arg_sizes_length; ++j)
            if(arg_total < unique_dflt_fcns[i].arg_sizes[j] && arg_total >= unique_dflt_fcns[i].arg_sizes[j] - unique_dflt_fcns[i].total_dflts[j])
              break;
          if(j == unique_dflt_fcns[i].arg_sizes_length) { // not at a defaulted arg that needs splicing in -- continue onto overload splicing
            i = unique_dflt_fcns_size;
            break;
          }

          // prefix dflt'd fcn invocation if its also overloaded
          for(k = 0; k < overload_fmacs_size; ++k)
            if(strcmp(overload_fmacs[k].name, unique_dflt_fcns[i].name) == 0) { // dflt'd fcn is also overloaded
              // prefix overloaded fcn/macro instance's name w/ reserved header & arg number
              sprintf(w, "DC__%d_%s", unique_dflt_fcns[i].arg_sizes[j], unique_dflt_fcns[i].name);
              w += strlen(w), r = scout;
              break;
            }
          // copy name if it wasn't prefixed as an overload
          if(k == overload_fmacs_size) while(*r != '\0' && r != scout) *w++ = *r++;
          bool IS_DECLASS_GENERATED = is_at_substring(unique_dflt_fcns[i].name, "DC_");

          // copy the args & splice in the appropriate dflt values at the end of the args list
          in_arg_scope = 1;
          arg_end = scout++; // move 1 past the opening '('
          while(*scout != '\0' && in_arg_scope > 0) { // ends w/ 'scout' pointing at ')'
            handle_string_char_brace_scopes(&in_a_string, &in_a_char, &ignore_arg, scout);
            if(!in_a_string && !in_a_char && *scout == '(') ++in_arg_scope;
            else if(!in_a_string && !in_a_char && *scout == ')') --in_arg_scope;
            else if(!in_a_string && !in_a_char && *scout == ',') arg_end = scout; // end of an argument
            if(*scout == '\0' || in_arg_scope <= 0) break;
            ++scout;
          }
          // splice in default args
          // if fcn generated by declass.c, splice in default args 1 position 
          // prior to the last arg (ie whats being passed to the "*this" ptr for methods)
          // => declass.c only generates fcns for user-def'd methods & macros for all else,
          //    thus any instances prefixed w/ "DC_" can be safley assumed to contain a "this"
          //    ptr at the end of the args list to splice default arg vals in front of
          if(IS_DECLASS_GENERATED) {
            while(r != arg_end + 1) *w++ = *r++; // copy args up to right before value being passed as '*this'
            dflt_args_left = unique_dflt_fcns[i].total_dflts[j] - unique_dflt_fcns[i].arg_sizes[j] + arg_total;
            for(k = dflt_args_left; k < unique_dflt_fcns[i].total_dflts[j]; ++k) {
              if(k > dflt_args_left) *w++ = ',';
              strcpy(w, unique_dflt_fcns[i].dflt_vals[j][k]);
              w += strlen(w);
            }
            next_arg = r; // confirm not at the end of the arg list prior to splicing in another ','
            while(IS_WHITESPACE(*next_arg)) ++next_arg;
            if(*next_arg != ')') *w++ = ','; // insert ',' btwn dflt value & '*this'
            while(r != scout) *w++ = *r++; // copy '*this' arg value
          } else { // NOT a declass.c generated fcn
            while(r != scout) *w++ = *r++; // copy args up to right before closing ')'
            dflt_args_left = unique_dflt_fcns[i].total_dflts[j] - unique_dflt_fcns[i].arg_sizes[j] + arg_total;
            for(k = dflt_args_left; k < unique_dflt_fcns[i].total_dflts[j]; ++k) {
              if(arg_total > 0 || k > dflt_args_left) *w++ = ',';
              strcpy(w, unique_dflt_fcns[i].dflt_vals[j][k]);
              w += strlen(w);
            }
          }
          *w++ = *r++; // copy closing ')' of args list
          break;
        }
      }
      // if DID find a valid matching dflt fcns (already prefixed it if overloaded above, thus force the next loop iteration)
      if(i < unique_dflt_fcns_size && *r != '\0') {
        *w++ = *r++;
        continue;
      }
      

      // -:- PARSE FOR OVERLOADS -:- 
      // check for potential fcn/macro overload
      for(i = 0; i < overload_fmacs_size; ++i) {
        // potential overloaded instance
        if(is_at_substring(r, overload_fmacs[i].name) && !VARCHAR(*(r+strlen(overload_fmacs[i].name)))) {
          // get number of args
          scout = r;
          while(VARCHAR(*scout))       ++scout; // skip past name
          while(IS_WHITESPACE(*scout)) ++scout; // skip past optional whitespace btwn name && '('
          
          // overload name invocation w/o any args passed btwn "()" afterwards -- COLA names
          // can NEVER to redefined/reassigned to ANY other variables in ANY other scope
          if(*scout != '(') {
            FLOOD_ZEROS(bad_code_buffer, BAD_CODE_BUFFER_LENGTH);
            get_invalid_code_snippet(read, scout, bad_code_buffer);
            throw_fatal_error_non_functional_invocation_of_overload_or_dflt_name(overload_fmacs[i].name, bad_code_buffer, __LINE__);
          }
          arg_total = count_args(scout, overload_fmacs[i].name, false, i);
          
          // confirm an overload instance with "arg_total" args was detected earlier
          for(j = 0; j < overload_fmacs[i].arg_sizes_length; ++j)
            if(arg_total == overload_fmacs[i].arg_sizes[j])
              break;

          // if overloaded instance invocation has an undefined # of args wrt "overload_fmacs",
          // ie no matching global definition/declaration/prototype found to couple w/ invocation
          if(j == overload_fmacs[i].arg_sizes_length) {
            FLOOD_ZEROS(bad_code_buffer, BAD_CODE_BUFFER_LENGTH);
            get_invalid_code_snippet(read, scout, bad_code_buffer);
            throw_fatal_error_undefined_arg_length_invocation(overload_fmacs[i].name, arg_total, bad_code_buffer, __LINE__);
          }

          // prefix overloaded fcn/macro instance's name w/ reserved header & arg number
          sprintf(w, "DC__%d_%s", arg_total, overload_fmacs[i].name);
          w += strlen(w), r = scout;
          break;
        }
      }
      if(i == overload_fmacs_size && *r != '\0') *w++ = *r++;
    } else
      *w++ = *r++;
  }
  *w = '\0';
}

/******************************************************************************
* DEFAULT VALUE "unique_dflt_fcns" (HAS DFLT FCN INSTANCES) STRUCT HELPER FCNS
******************************************************************************/

// checks whether same fcn name already in "unique_dflt_fcns"
bool name_not_in_unique_dflt_fcns(char *name) {
  for(int i = 0; i < unique_dflt_fcns_size; ++i) 
    if(strcmp(unique_dflt_fcns[i].name, name) == 0) return false;
  return true;
}

// returns idx of fcn arg-defaulted fcn instance if instance doesn't already have 
// "all_dflt_fcns[all_dflt_fcns_idx].total_args" in unique_dflt_fcns's "arg_sizes[]", & returns 
// -1 if already found (ie if already stored its name & arg # instance from a prototype)
int unique_dflt_fcns_instance_idx(int all_dflt_fcns_idx) {
  for(int j = 0, k; j < unique_dflt_fcns_size; ++j) 
    if(strcmp(unique_dflt_fcns[j].name, all_dflt_fcns[all_dflt_fcns_idx].fcn_name) == 0) {
      // search instance's arg lengths for "all_dflt_fcns[all_dflt_fcns_idx].total_args"
      for(k = 0; k < unique_dflt_fcns[j].arg_sizes_length; ++k)
        if(all_dflt_fcns[all_dflt_fcns_idx].total_args == unique_dflt_fcns[j].arg_sizes[k]) break;
      // if arg count not found, return instance idx to 
      // add # of args as a new overload to "unique_dflt_fcns" instance's "arg_sizes[]"
      return (k == unique_dflt_fcns[j].arg_sizes_length) ? j : -1;
    }
  return -1;
}

// returns the # of dflt'd args for the total # of args instance in "unique_dflt_fcns[unique_dflt_fcns_idx]"
int total_dflts_for_ARG_SIZE_fcn_instance_in_UNIQUE_DFLT_FCNS(int arg_size, int unique_dflt_fcns_idx) {
  for(int i = 0; i < unique_dflt_fcns[unique_dflt_fcns_idx].arg_sizes_length; ++i) // for each overloaded & dflt'd instance's arg length
    if(unique_dflt_fcns[unique_dflt_fcns_idx].arg_sizes[i] == arg_size) // if same number of args, found the fcn instance
      return unique_dflt_fcns[unique_dflt_fcns_idx].total_dflts[i]; // return how many dflt'd arg values the fcn instance has
  return 0;
}

/******************************************************************************
* DEFAULT VALUE FILTER FCN FROM "ALL_DFLT_FCNS" INTO "UNIQUE_DFLT_FCNS" STRUCT
******************************************************************************/

// convert "all_dflt_fcns" struct to "unique_dflt_fcns" struct (disregarding non-defaulted fcn instances)
void filter_defaulted_fcns_from_ALL_DFLT_FCNS_to_UNIQUE_DFLT_FCNS() {
  int i, j, dflts_idx, per_fcn_idx;
  for(i = 0; i < all_dflt_fcns_size; ++i) {
    if(all_dflt_fcns[i].total_dflts > 0 && name_not_in_unique_dflt_fcns(all_dflt_fcns[i].fcn_name)) {
      // new function name w/ dflt args instance
      strcpy(unique_dflt_fcns[unique_dflt_fcns_size].name, all_dflt_fcns[i].fcn_name);       // copy new fcn name
      unique_dflt_fcns[unique_dflt_fcns_size].arg_sizes[0] = all_dflt_fcns[i].total_args;    // copy # of total args
      unique_dflt_fcns[unique_dflt_fcns_size].arg_sizes_length = 1; // 1st instance of arg list size
      unique_dflt_fcns[unique_dflt_fcns_size].total_dflts[0] = all_dflt_fcns[i].total_dflts; // copy # of defaulted args
      for(j = 0; j < all_dflt_fcns[i].total_dflts; ++j) { // copy default arg values & idxs
        strcpy(unique_dflt_fcns[unique_dflt_fcns_size].dflt_vals[0][j], all_dflt_fcns[i].dflt_vals[j]);
        unique_dflt_fcns[unique_dflt_fcns_size].dflt_idxs[0][j] = all_dflt_fcns[i].dflt_idxs[j];
      }
      ++ unique_dflt_fcns_size;
    } else if(all_dflt_fcns[i].total_dflts && (dflts_idx = unique_dflt_fcns_instance_idx(i)) != -1) {
      // existing function name w/ dflt args has a new arg-length overload instance
      per_fcn_idx = unique_dflt_fcns[dflts_idx].arg_sizes_length;
      unique_dflt_fcns[dflts_idx].arg_sizes[per_fcn_idx] = all_dflt_fcns[i].total_args;    // copy # of total args
      unique_dflt_fcns[dflts_idx].total_dflts[per_fcn_idx] = all_dflt_fcns[i].total_dflts; // copy # of defaulted args
      for(j = 0; j < all_dflt_fcns[i].total_dflts; ++j) { // copy default arg values & idxs
        strcpy(unique_dflt_fcns[dflts_idx].dflt_vals[per_fcn_idx][j], all_dflt_fcns[i].dflt_vals[j]);
        unique_dflt_fcns[dflts_idx].dflt_idxs[per_fcn_idx][j] = all_dflt_fcns[i].dflt_idxs[j];
      }
      ++ unique_dflt_fcns[dflts_idx].arg_sizes_length;
    }
  }
}

/******************************************************************************
* DEFAULT VALUE FCNS CONFIRMING DFLT != PRIOR NON-DFLT, IN MACRO, OR AMBIGUOUS 
******************************************************************************/

// overloaded & dflt'd fcns must not be ambiguous, IE overload definitions must satisfy 1 of the following: 
//   (1) an overload's # of non-default args must exceed the # of cumulative
//       args (both dflt & not) of all other overloaded instances
//   (2) an overload's # of cumulative args (both dflt & not) must be less
//       than the # of non-default args of all other overloaded instances
bool ambiguous_overload(int overload_fmacs_dflts, int overload_fmacs_arg_size, int unique_dflt_fcns_arg_size, int unique_dflt_fcns_dflts) {
  int overload_fmacs_nondflts   = overload_fmacs_arg_size - overload_fmacs_dflts;
  int unique_dflt_fcns_nondflts = unique_dflt_fcns_arg_size - unique_dflt_fcns_dflts;
  bool more_nondflt_args_than_total_args = (overload_fmacs_nondflts > unique_dflt_fcns_arg_size);
  bool less_total_args_than_nondflt_args = (overload_fmacs_arg_size < unique_dflt_fcns_nondflts);
  return !(more_nondflt_args_than_total_args || less_total_args_than_nondflt_args);
}

// confirm no ambigious overloaded definitions wrt assigned default values causing indeterminate fcn invocations
void confirm_no_ambiguous_overloaded_default_values() {
  int overload_fmacs_idx = 0, unique_dflt_fcns_idx, i, j;
  int overload_fmacs_total_dflts;   // # of dflt args for an overload instance
  int unique_dflt_fcns_total_dflts; // # of dflt args for a defaulted instance
  int overload_fmacs_arg_size;      // # of total args for an overload instance
  int unique_dflt_fcns_arg_size;    // # of total args for a defaulted instance
  for(; overload_fmacs_idx < overload_fmacs_size; ++overload_fmacs_idx) // for each overload
    for(unique_dflt_fcns_idx = 0; unique_dflt_fcns_idx < unique_dflt_fcns_size; ++unique_dflt_fcns_idx) { // for each defaulted fcn
      #define current_fmac overload_fmacs[overload_fmacs_idx]
      #define current_dflt unique_dflt_fcns[unique_dflt_fcns_idx]
      // if at least 1 overload instance also has a dflt arg value
      if(strcmp(current_fmac.name, current_dflt.name) == 0)  
        /* at this point exact same arg-length fcn duplicates would have triggered a "duplicate args length" 
         * fatal error, thus all arg lengths -- defaulted or not -- are unique */
        for(i = 0; i < current_fmac.arg_sizes_length; ++i) // for each overload instance (via the differnt unique arg lengths registered)
          for(j = 0; j < current_dflt.arg_sizes_length; ++j) { // for each dflt instance
            // same fcn name & arg length == same fcn instance
            if(current_fmac.arg_sizes[i] == current_dflt.arg_sizes[j]) continue; // don't compare a fcn to itself
            overload_fmacs_total_dflts = total_dflts_for_ARG_SIZE_fcn_instance_in_UNIQUE_DFLT_FCNS(current_fmac.arg_sizes[i], unique_dflt_fcns_idx);
            overload_fmacs_arg_size    = current_fmac.arg_sizes[i];
            unique_dflt_fcns_arg_size     = current_dflt.arg_sizes[j];
            unique_dflt_fcns_total_dflts  = current_dflt.total_dflts[j];
            
            if(ambiguous_overload(overload_fmacs_total_dflts, overload_fmacs_arg_size, unique_dflt_fcns_arg_size, unique_dflt_fcns_total_dflts))
              throw_fatal_error_ambiguous_overload(current_fmac.name, overload_fmacs_total_dflts, 
                overload_fmacs_arg_size, unique_dflt_fcns_arg_size, unique_dflt_fcns_total_dflts, __LINE__);
          }
    }
  #undef current_fmac
  #undef current_dflt
}

// confirms any/all dflt values were defined at the end of the fcn's arg list & not in a macro
void confirm_valid_default_values_position(int arg_total, char *function_name, bool is_a_macro) {
  #define current_dflt all_dflt_fcns[all_dflt_fcns_size] 
  if(current_dflt.total_dflts == 0) return; // no defaulted values found
  // declass.c methods have '*this' spliced as their last arg (unbeknownst to the user), 
  // thus enable default values args 1 position prior to the end of the args list rather than 
  // the very end (as opposed to every other non-"DC_" prefixed function w/ dflt values)
  bool IS_DECLASS_GENERATED = is_at_substring(function_name, "DC_"); 
  const int earliest_dflt_idx = arg_total - current_dflt.total_dflts - IS_DECLASS_GENERATED; 
  for(int i = 0; i < current_dflt.total_dflts; ++i)
    if(current_dflt.dflt_idxs[i] < earliest_dflt_idx || is_a_macro)
      throw_fatal_error_invalid_default_arg_value(function_name, is_a_macro, __LINE__);
  #undef current_dflt
}

/******************************************************************************
* DEFAULT VALUE STORING IN "all_dflt_fcns" & WHITING-OUT FROM ARG LIST FUNCTION
******************************************************************************/

// given a ptr in an function's arg list, reads & stores default value in "all_dflt_fcns" struct,
// returns length of the default value + 1 to whitespace the "= <dfltVal>" in the args list
int store_dflt_value(int current_arg_idx, char *p) {
  int idx = 0, dflt_val_length = 1; // "1" to account for the '='
  char *scout = p + 1;
  // more readble to repn. current "fcn" dflt-tracking instance in "all_dflt_fcns"
  #define current_dflt all_dflt_fcns[all_dflt_fcns_size] 
  FLOOD_ZEROS(current_dflt.dflt_vals[current_arg_idx], MAX_ARGS_PER_FCN);
  // store defaulted arg's idx relative to ithe fcn's entire args list
  current_dflt.dflt_idxs[current_dflt.total_dflts] = current_arg_idx; 
  // skip optional whitespace btwn '=' && dflt value
  while(IS_WHITESPACE(*scout)) ++scout, ++dflt_val_length; 
  // copy default value
  while(*scout != '\0' && *scout != ',' && *scout != ')')
    current_dflt.dflt_vals[current_dflt.total_dflts][idx++] = *scout++, ++dflt_val_length; 
  current_dflt.dflt_vals[current_dflt.total_dflts][idx] = '\0';
  if(idx > 0) { // trim possible whitespaces following the default value
    scout = &current_dflt.dflt_vals[current_dflt.total_dflts][idx-1];
    while(IS_WHITESPACE(*scout)) *scout-- = '\0'; 
  }
  ++ current_dflt.total_dflts;
  #undef current_dflt
  return dflt_val_length;
}
