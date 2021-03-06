/* DECLASSIFIED: declass_SampleExec.c
 * Email jrandleman@scu.edu or see https://github.com/jrandleman for support */
int DC__NDTR = 1;
#define immortal // immortal keyword active
/****************************** SMRTPTR.H START ******************************/
// Source: https://github.com/jrandleman/C-Libraries/tree/master/Smart-Pointer
#ifndef SMRTPTR_H_
#define SMRTPTR_H_
#include <stdio.h>
#include <stdlib.h>
// garbage collector & smart pointer storage struct
static struct SMRTPTR_GARBAGE_COLLECTOR {
  long len, max; // current # of ptrs && max capacity
  void **ptrs;   // unique ptr set to free all smrt ptrs
} SMRTPTR_GC = {-1};
// invoked by atexit to free all ctor-alloc'd memory
static void smrtptr_free_all() {
  int i = 0;
  for(; i < SMRTPTR_GC.len; ++i) free(SMRTPTR_GC.ptrs[i]);
  if(SMRTPTR_GC.len > 0) free(SMRTPTR_GC.ptrs);
  // if(SMRTPTR_GC.len > 0) printf("FREED %ld SMART POINTERS!\n", SMRTPTR_GC.len); // optional
  SMRTPTR_GC.len = 0;
}
// throws invalid allocation errors
static void smrtptr_throw_bad_alloc(char *alloc_type, char *smrtptr_h_fcn) {
  fprintf(stderr, "\n-:- \033[1m\033[31mERROR\033[0m COULDN'T %s MEMORY FOR SMRTPTR.H'S %s -:-\n\n", alloc_type, smrtptr_h_fcn);
  fprintf(stderr, "-:- FREEING ALLOCATED MEMORY THUS FAR AND TERMINATING PROGRAM -:-\n\n");
  exit(EXIT_FAILURE); // still frees any ptrs allocated thus far
}
// acts like assert, but exits rather than abort to free smart pointers
#ifndef DECLASS_NDEBUG
#define smrtassert(condition) ({\
  if(!(condition)) {\
    fprintf(stderr, "\n\033[1m\033[31mERROR\033[0m Smart Assertion failed: (%s), function %s, file %s, line %d.\n", #condition, __func__, __FILE__, __LINE__);\
    fprintf(stderr, ">> Freeing Allocated Smart Pointers & Terminating Program.\n\n");\
    exit(EXIT_FAILURE);\
  }\
})
#else
#define smrtassert(condition)
#endif
// smrtptr stores ptr passed as arg to be freed atexit
void smrtptr(void *ptr) {
  // free ptrs atexit
  atexit(smrtptr_free_all);
  // malloc garbage collector
  if(SMRTPTR_GC.len == -1) {
    SMRTPTR_GC.ptrs = malloc(sizeof(void *) * 10);
    if(!SMRTPTR_GC.ptrs) {
      fprintf(stderr, "\n-:- \033[1m\033[31mERROR\033[0m COULDN'T MALLOC MEMORY TO INITIALIZE SMRTPTR.H'S GARBAGE COLLECTOR -:-\n\n");
      exit(EXIT_FAILURE);
    }
    SMRTPTR_GC.max = 10, SMRTPTR_GC.len = 0;
  }
  // reallocate if "max" ptrs already added
  if(SMRTPTR_GC.len == SMRTPTR_GC.max) {
    SMRTPTR_GC.max *= SMRTPTR_GC.max;
    void **SMRTPTR_TEMP = realloc(SMRTPTR_GC.ptrs, sizeof(void *) * SMRTPTR_GC.max);
    if(!SMRTPTR_TEMP) smrtptr_throw_bad_alloc("REALLOC", "GARBAGE COLLECTOR");
    SMRTPTR_GC.ptrs = SMRTPTR_TEMP;
  }
  // add ptr to SMRTPTR_GC if not already present (ensures no double-freeing)
  int i = 0;
  for(; i < SMRTPTR_GC.len; ++i) if(SMRTPTR_GC.ptrs[i] == ptr) return;
  SMRTPTR_GC.ptrs[SMRTPTR_GC.len++] = ptr;
  // printf("SMART POINTER #%ld STORED!\n", SMRTPTR_GC.len); // optional
}
// malloc's a pointer, stores it in the garbage collector, then returns ptr
void *smrtmalloc(size_t alloc_size) {
  void *smtr_malloced_ptr = malloc(alloc_size);
  if(smtr_malloced_ptr == NULL) smrtptr_throw_bad_alloc("MALLOC", "SMRTMALLOC FUNCTION");
  smrtptr(smtr_malloced_ptr);
  return smtr_malloced_ptr;
}
// calloc's a pointer, stores it in the garbage collector, then returns ptr
void *smrtcalloc(size_t alloc_num, size_t alloc_size) {
  void *smtr_calloced_ptr = calloc(alloc_num, alloc_size);
  if(smtr_calloced_ptr == NULL) smrtptr_throw_bad_alloc("CALLOC", "SMRTCALLOC FUNCTION");
  smrtptr(smtr_calloced_ptr);
  return smtr_calloced_ptr;
}
// realloc's a pointer, stores it anew in the garbage collector, then returns ptr
// compatible both "smart" & "dumb" ptrs!
void *smrtrealloc(void *ptr, size_t realloc_size) {
  int i = 0;
  void *smtr_realloced_ptr;
  // realloc a "smart" ptr already in garbage collector
  for(; i < SMRTPTR_GC.len; ++i)
    if(SMRTPTR_GC.ptrs[i] == ptr) {
      smtr_realloced_ptr = realloc(ptr, realloc_size); // frees ptr in garbage collector
      if(smtr_realloced_ptr == NULL) smrtptr_throw_bad_alloc("REALLOC", "SMRTREALLOC FUNCTION");
      SMRTPTR_GC.ptrs[i] = smtr_realloced_ptr; // point freed ptr at realloced address
      // printf("SMART POINTER REALLOC'D!\n"); // optional
      return smtr_realloced_ptr;
    }
  // realloc a "dumb" ptr then add it to garbage collector
  smtr_realloced_ptr = realloc(ptr, realloc_size);
  if(smtr_realloced_ptr == NULL) smrtptr_throw_bad_alloc("REALLOC", "SMRTREALLOC FUNCTION");
  smrtptr(smtr_realloced_ptr);
  return smtr_realloced_ptr;
}
// prematurely frees ptr arg prior to atexit (if exists)
void smrtfree(void *ptr) {
  int i = 0, j;
  for(; i < SMRTPTR_GC.len; ++i) // find ptr in garbage collector
    if(SMRTPTR_GC.ptrs[i] == ptr) {
      free(ptr);
      for(j = i; j < SMRTPTR_GC.len - 1; ++j) // shift ptrs down
        SMRTPTR_GC.ptrs[j] = SMRTPTR_GC.ptrs[j + 1];
      SMRTPTR_GC.len--;
      // printf("SMART POINTER FREED!\n"); // optional
      return;
    }
}
#endif
/******************************* SMRTPTR.H END *******************************/

 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/******************************** CLASS START ********************************/
/* "Student" CLASS DEFAULT VALUE MACRO CONSTRUCTORS: */
#define DC__Student_CTOR(DC_THIS) ({DC_THIS = DC__Student_DFLT();})
#define DC__Student_ARR(DC_ARR) ({\
  for(int DC__Student_IDX=0;DC__Student_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__Student_IDX)\
    DC__Student_CTOR(DC_ARR[DC__Student_IDX]);\
})
#define DC__4_DC__Student_UCTOR_ARR(DC_ARR, DC___A1_Student, DC___A2_Student, DC___A3_Student) ({\
  for(int DC__Student_UCTOR_IDX=0;DC__Student_UCTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__Student_UCTOR_IDX)\
    DC__4_DC_Student_(DC___A1_Student, DC___A2_Student, DC___A3_Student, &DC_ARR[DC__Student_UCTOR_IDX]);\
})
#define DC__3_DC__Student_UCTOR_ARR(DC_ARR, DC___A1_Student, DC___A2_Student) ({\
  for(int DC__Student_UCTOR_IDX=0;DC__Student_UCTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__Student_UCTOR_IDX)\
    DC__3_DC_Student_(DC___A1_Student, DC___A2_Student, &DC_ARR[DC__Student_UCTOR_IDX]);\
})
/* "Student" CLASS OBJECT ARRAY MACRO DESTRUCTOR: */
#define DC__Student_UDTOR_ARR(DC_ARR) ({\
  for(int DC__Student_UDTOR_IDX=0;DC__Student_UDTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__Student_UDTOR_IDX)\
		if(DC_ARR[DC__Student_UDTOR_IDX].DC_DTR){DC__NOT_Student_(&DC_ARR[DC__Student_UDTOR_IDX]);DC_ARR[DC__Student_UDTOR_IDX].DC_DTR=NULL;}\
})

/* "Student" CLASS CONVERTED TO STRUCT: */
typedef struct DC_Student {
	int *DC_DTR;
  char *fullname;
  char school[15];
  int year;
  long studentId;
  char *(*copy_fcnPtr)();
  struct grade_info {
    char major[50];
    float out_of;
    float gpa;
  } grades;


} Student;
Student DC__Student_DFLT(){
	Student this={&DC__NDTR,smrtmalloc(sizeof(char)*50),"SCU",14,0,strcpy,{"Computer Science Engineering", 4.0},};
	return this;
}

/* DEFAULT PROVIDED "Student" CLASS CONSTRUCTOR/DESTRUCTOR: */
#define DC__3_DC__DUMMY_Student(DC___D1_Student, DC___D2_Student, DC___D3_Student)({\
	Student DC__Student__temp;\
	DC__Student_CTOR(DC__Student__temp);\
	DC__4_DC_Student_(DC___D1_Student, DC___D2_Student, DC___D3_Student,  &DC__Student__temp);\
})
#define DC__2_DC__DUMMY_Student(DC___D1_Student, DC___D2_Student)({\
	Student DC__Student__temp;\
	DC__Student_CTOR(DC__Student__temp);\
	DC__3_DC_Student_(DC___D1_Student, DC___D2_Student,  &DC__Student__temp);\
})

/* "Student" CLASS METHODS SPLICED OUT: */
  void DC_Student_assignName(char *fname, Student *this) {
    this->copy_fcnPtr(this->fullname, fname);
  }
  void DC_Student_getName(char *name, Student *this) {
    strcpy(name, this->fullname);
  }
  void DC_Student_assignGpa(float grade, Student *this) { this->grades.gpa = grade; }
  void DC_Student_assignId(long id, Student *this) { this->studentId = id; }
  long DC_Student_getId(Student *this) { return this->studentId; }
  void DC_Student_show(Student *this) {
    if(DC_Student_getId(this)) {
      printf("Name: %s, School: %s, Year: %d, id: %ld", this->fullname, this->school, this->year, this->studentId);
      printf(" Major: %s, GPA: %.1f/%.1f\n", this->grades.major, this->grades.gpa, this->grades.out_of);
    }
  }
  Student DC__4_DC_Student_(char *userName, long id, float gpa, Student *this) {
    DC_Student_assignName(userName, this);
    DC_Student_assignId(id, this);
    DC_Student_assignGpa(gpa, this);
  	return *this;
	}
  Student DC__3_DC_Student_(float gpa, char *userName                             , Student *this) {
    DC_Student_assignName(userName, this);
    DC_Student_assignId(1000000, this);
    DC_Student_assignGpa(gpa, this);
  	return *this;
	}
  void DC__NOT_Student_(Student *this) {
    printf("\"Student\" object named \"%s\" Destroyed!\n", this->fullname);
  }
  Student DC_Student_createAStudent(char *name, long id, float gpa, Student *this) {
    Student methodMadeStudent; DC__Student_CTOR(methodMadeStudent); methodMadeStudent.DC_DTR=&DC__NDTR;
    DC_Student_assignName(name, &methodMadeStudent);
    DC_Student_assignId(id, &methodMadeStudent);
    DC_Student_assignGpa(gpa, &methodMadeStudent);
    return methodMadeStudent;
  }
  void DC_Student_swap(Student *blankStudent, Student *this) {


    immortal Student temp = *this; temp.DC_DTR=&DC__NDTR;
    *this = *blankStudent;
    *blankStudent = temp;
  }
/********************************* CLASS END *********************************/


/******************************** CLASS START ********************************/
/* "College" CLASS DEFAULT VALUE MACRO CONSTRUCTORS: */
#define DC__College_CTOR(DC_THIS) ({DC_THIS = DC__College_DFLT();\
	DC__Student_ARR(DC_THIS.body);})
#define DC__College_ARR(DC_ARR) ({\
  for(int DC__College_IDX=0;DC__College_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__College_IDX)\
    DC__College_CTOR(DC_ARR[DC__College_IDX]);\
})
#define DC__College_UCTOR_ARR(DC_ARR) ({\
  for(int DC__College_UCTOR_IDX=0;DC__College_UCTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__College_UCTOR_IDX)\
DC_College_(&DC_ARR[DC__College_UCTOR_IDX]);\
})
/* "College" CLASS OBJECT ARRAY MACRO DESTRUCTOR: */
#define DC__College_UDTOR_ARR(DC_ARR) ({\
  for(int DC__College_UDTOR_IDX=0;DC__College_UDTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__College_UDTOR_IDX)\
		if(DC_ARR[DC__College_UDTOR_IDX].DC_DTR){DC__NOT_College_(&DC_ARR[DC__College_UDTOR_IDX]);DC_ARR[DC__College_UDTOR_IDX].DC_DTR=NULL;}\
})

/* "College" CLASS CONVERTED TO STRUCT: */
typedef struct DC_College {
	int *DC_DTR;
  immortal Student body[10];
  char name[20];
  int foundingYear;
  char state[3];

} College;
College DC__College_DFLT(){
	College this={&DC__NDTR,{0},{0},0,"CA",};
	return this;
}

/* DEFAULT PROVIDED "College" CLASS CONSTRUCTOR/DESTRUCTOR: */
College DC_College_(College*this){return*this;}
#define DC__DUMMY_College()({\
	College DC__College__temp;\
	DC__College_CTOR(DC__College__temp);\
	DC_College_(&DC__College__temp);\
})

/* "College" CLASS METHODS SPLICED OUT: */
  void DC_College_addName(char *uniName, College *this) { strcpy(this->name, uniName); }
  void DC_College_addFoundingAndName(int founded, char *name, College *this) {
    if(founded < 0) {printf("A school founded in BC? Really?\n");}
    this->foundingYear = founded;


    DC_College_addName(name, this);
  }
  void DC_College_addStudents(char names[10][20], float gpas[10], long ids[10], College *this) {
    for(int i = 0; i < 10; ++i) {
      if(i % 2 == 0)
        {strcpy(this->body[i].fullname, names[i]);}
      else
        {DC_Student_assignName(names[i], &(this->body[i]));}
      DC_Student_assignGpa(gpas[i], &(this->body[i]));
      DC_Student_assignId(ids[i], &(this->body[i]));
    }
  }
  void DC_College_show(College *this) {
    printf("College Name: %s, State: %s, Year Founded: %d.\n", this->name, this->state, this->foundingYear);
    printf("%s's Students:\n", this->name);
    for(int i = 0; i < 10; ++i) {
      printf("\tStudent No%d: ", i + 1);
      DC_Student_show(&(this->body[i]));
    }
  }
  void DC__NOT_College_(College *this) {
    printf("\"College\" object named \"%s\" Destroyed!\n", this->name);
  }
/********************************* CLASS END *********************************/


/******************************** CLASS START ********************************/
/* "Region" CLASS DEFAULT VALUE MACRO CONSTRUCTORS: */
#define DC__Region_CTOR(DC_THIS) ({DC_THIS = DC__Region_DFLT();\
	DC__College_ARR(DC_THIS.schools);\
	DC__Student_CTOR(DC_THIS.topStudent);\
	DC__4_DC_Student_("Stephen Prata", 12121212, 4.0, &DC_THIS.topStudent);\
	DC__Student_ARR(DC_THIS.second3rd4thBestStudents);\
	DC__4_DC__Student_UCTOR_ARR(DC_THIS.second3rd4thBestStudents, "John Doe", 11111110, 8.0);})
#define DC__Region_ARR(DC_ARR) ({\
  for(int DC__Region_IDX=0;DC__Region_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__Region_IDX)\
    DC__Region_CTOR(DC_ARR[DC__Region_IDX]);\
})
#define DC__Region_UCTOR_ARR(DC_ARR) ({\
  for(int DC__Region_UCTOR_IDX=0;DC__Region_UCTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__Region_UCTOR_IDX)\
DC_Region_(&DC_ARR[DC__Region_UCTOR_IDX]);\
})
/* "Region" CLASS OBJECT ARRAY MACRO DESTRUCTOR: */
#define DC__Region_UDTOR_ARR(DC_ARR) ({\
  for(int DC__Region_UDTOR_IDX=0;DC__Region_UDTOR_IDX<(sizeof(DC_ARR)/sizeof(DC_ARR[0]));++DC__Region_UDTOR_IDX)\
		if(DC_ARR[DC__Region_UDTOR_IDX].DC_DTR){DC__NOT_Region_(&DC_ARR[DC__Region_UDTOR_IDX]);DC_ARR[DC__Region_UDTOR_IDX].DC_DTR=NULL;}\
})

/* "Region" CLASS CONVERTED TO STRUCT: */
typedef struct DC_Region {
	int *DC_DTR;
  College schools[2];
  int totalSchools;
  char regionName[20];
  Student topStudent;
  Student second3rd4thBestStudents[3];
} Region;
Region DC__Region_DFLT(){
	Region this={&DC__NDTR,{0},0,{0},{0},{0},};
	return this;
}

/* DEFAULT PROVIDED "Region" CLASS CONSTRUCTOR/DESTRUCTOR: */
void DC__NOT_Region_(Region *this) {
	DC__College_UDTOR_ARR(this->schools);
	DC__NOT_Student_(&(this->topStudent));
	DC__Student_UDTOR_ARR(this->second3rd4thBestStudents);
}
Region DC_Region_(Region*this){return*this;}
#define DC__DUMMY_Region()({\
	Region DC__Region__temp;\
	DC__Region_CTOR(DC__Region__temp);\
	DC_Region_(&DC__Region__temp);\
})

/* "Region" CLASS METHODS SPLICED OUT: */
  void DC_Region_setRegionName(char *name, Region *this) { strcpy(this->regionName, name); }
  void DC_Region_addSchool(College school, Region *this) {
    if(this->totalSchools == 2) {
      printf("Region Maxed out of Schools!\n");
      return;
    }
    this->schools[this->totalSchools++] = school;

    DC_Student_assignName("CAMERON", &(this->schools[this->totalSchools-1].body[this->totalSchools - this->totalSchools]));
  }
  void DC_Region_show(Region *this) {
    printf("Region Name: %s, Total Schools: %d.\n", this->regionName, this->totalSchools);
    for(int i = 0; i < this->totalSchools; ++i) {
      printf("School No%d:\n", i + 1);
      DC_College_show(&(this->schools[i]));
    }
  }
  void DC_Region_showTopStudents(Region *this) {
    printf("\tThe %s Region's Top Student:\n\t\t", this->regionName);
    DC_Student_show(&(this->topStudent));
    printf("\tThe Next 3 Runner-Ups:\n");
    for(int i = 0; i < 3; ++i) {
      printf("\t\t");
      DC_Student_show(&(this->second3rd4thBestStudents[i]));
    }
  }
/********************************* CLASS END *********************************/




void showAllNames(Student person, Student *sharpStudent, Student people[6]) {
  char personName[30], people3Name[30], sharpStudentName[30];
  DC_Student_getName(personName, &person);
  DC_Student_getName(people3Name, &people[3]);
  DC_Student_getName(sharpStudentName, sharpStudent);
  printf("person's Name: %s, people[3]'s Name: %s", personName, people3Name);
  printf(", *sharpStudent's Name: %s\n", sharpStudentName);
}


College createCollege(char *name, int foundingYear) {


  College someUniversity; DC__College_CTOR(someUniversity); someUniversity.DC_DTR=&DC__NDTR;
  DC_College_addName(name, &someUniversity);
  someUniversity.foundingYear = foundingYear;
  return someUniversity;
}


int main() {


  printf("Working with a single \"Student\" object:\n");


  Student jordanCR; DC__Student_CTOR(jordanCR); jordanCR.DC_DTR=&DC__NDTR;
  DC_Student_assignId(1524026, &jordanCR);
  long myId = DC_Student_getId(&jordanCR);
  if(2 * DC_Student_getId(&jordanCR) > 1000)
    {printf("\tStudent object: school = %s, id = %ld, myId: %ld\n", jordanCR.school, jordanCR.studentId, myId);}
  DC_Student_assignName("Jordan Randleman", &jordanCR);
  DC_Student_assignGpa(3.9, &jordanCR);
  printf("\t");
  DC_Student_show(&jordanCR);


  printf("\nWorking with an single \"Student\" object initialized via its constructor:\n");
  Student koenR; DC__Student_CTOR(koenR); DC__4_DC_Student_("Koen Randleman", 1122334, 4.0, &koenR); koenR.DC_DTR=&DC__NDTR;
  printf("\t");
  DC_Student_show(&koenR);


  printf("\nWorking with an single \"Student\" object initialized via its overloaded constructor:\n");
  Student luluR; DC__Student_CTOR(luluR); DC__3_DC_Student_(5.0, "Louis Randleman", &luluR); luluR.DC_DTR=&DC__NDTR;
  printf("\t");
  DC_Student_show(&luluR);


  printf("\nWorking with an \"Student\" object constructed via an overloaded ctor w/ a default \"name\" arg value:\n");
  Student charmR; DC__Student_CTOR(charmR); DC__3_DC_Student_(5.5,"\"Default Student Name\"", &charmR); charmR.DC_DTR=&DC__NDTR;
  printf("\t");
  DC_Student_show(&charmR);


  printf("\nUsing the \"dummy\" constructor:\n");
  Student tessaR = DC__3_DC__DUMMY_Student("Tessa Randleman", 1678, 4.0); tessaR.DC_DTR=&DC__NDTR;
  DC_Student_show(&tessaR);


  printf("\nWorking with an array of 6 \"Student\" objects:\n");
  Student class[6]; DC__Student_ARR(class);
  char names[6][20] = {"Cameron", "Sidd", "Austin", "Sabiq", "Tobias", "Gordon"};
  for(int i = 0; i < 6; ++i) {
    DC_Student_assignName(names[i], &class[i]);
    DC_Student_assignId(i * 256 + 1000000, &class[i]);
    DC_Student_assignGpa(i * 0.75, &class[i]);
  }
  for(int i = 0; i < 6; ++i) {
    printf("\t");
    DC_Student_show(&class[i]);
  }


  printf("\nWorking with a constructor to initialize an array of 6 \"Student\" objects:\n");


  Student group[6]; DC__Student_ARR(group); DC__4_DC__Student_UCTOR_ARR(group, "group_student", 1111111, 3.0);
  for(int i = 0; i < 6; ++i) {
    printf("\t");
    DC_Student_show(&group[i]);
  }


  printf("\nWorking with a \"Student\" object pointer:\n");
  Student *JT = &class[5];
  DC_Student_assignName("JT", JT);
  printf("\t");
  DC_Student_show(JT);
  long jtId = DC_Student_getId(JT);
  printf("\tJT \"Student\" object pointer's ID: %ld\n", jtId);


  printf("\nAllocating and constructing a \"Student\" object pointer in a single line:\n");
  Student *Alex = smrtmalloc(sizeof(Student)); if(Alex){ DC__Student_CTOR((*Alex));Alex->DC_DTR=&DC__NDTR; DC__4_DC_Student_("Alex", 88888, 4.0, Alex);}
  DC_Student_show(Alex);


  printf("\nExplicitly invoking a \"Student\" object's destructor:\n");
  if(Alex&&Alex->DC_DTR){DC__NOT_Student_(Alex);Alex=NULL;}




  printf("\nWorking with contained \"Student\" objects within a \"College\" object:\n");


  College Scu; DC__College_CTOR(Scu); DC_College_(&Scu); Scu.DC_DTR=&DC__NDTR;
  DC_College_addFoundingAndName(1851, "SCU", &Scu);
  char studentNames[10][20] = {"Cameron","Sidd","Austin","Sabiq","Tobias","Gordon","Jason","Ronnie","Kyle","Peter"};
  float studentGpas[10] = {4.0, 3.9, 4.0, 3.9, 4.0, 3.9, 4.0, 3.9, 4.0, 3.9};
  long studentIds[10] = {4000000,3900000,3800000,3700000,3600000,3500000,3600000,3700000,3800000,3900000};
  DC_College_addStudents(studentNames, studentGpas, studentIds, &Scu);


  DC_College_show(&Scu);


  printf("\nPassing an object pointer, array, & value to another function:\n");
  printf("\t");
  showAllNames(jordanCR, JT, class);


  printf("\nHaving a function make & return a \"College\" object:\n");


  College SantaClara = createCollege("Santa Clara", 1851); SantaClara.DC_DTR=&DC__NDTR;
  printf("\tSantaClara \"College\" object Name: %s, State: %s, Year Founded: %d\n",
    SantaClara.name, SantaClara.state, SantaClara.foundingYear);


  Student willAR = DC_Student_createAStudent("Will Randleman", 1524027, 4.0, &jordanCR); willAR.DC_DTR=&DC__NDTR;
  printf("\nHaving a method make & return a \"Student\" object:\n");
  printf("\t");
  DC_Student_show(&willAR);


  printf("\nHaving a method swap 2 \"Student\" objects via '*this' pointer in method:\n");
  Student jowiR; DC__Student_CTOR(jowiR); jowiR.DC_DTR=&DC__NDTR;
  DC_Student_assignName("Jowi Randleman", &jowiR);
  DC_Student_assignId(5052009, &jowiR);
  DC_Student_assignGpa(100, &jowiR);
  printf("\tjowiR object pre-swap:   ");
  DC_Student_show(&jowiR);
  printf("\twillAR object pre-swap:  ");
  DC_Student_show(&willAR);
  DC_Student_swap(&jowiR, &willAR);
  printf("\tjowiR object post-swap:  ");
  DC_Student_show(&jowiR);
  printf("\twillAR object post-swap: ");
  DC_Student_show(&willAR);


  printf("\nMulti-Layer Object Containment - \"Region\" object containing");
  printf(" a \"College\" object array each containing a \"Student\" object array:\n");


  immortal Region SiliconValley; DC__Region_CTOR(SiliconValley); SiliconValley.DC_DTR=&DC__NDTR;
  DC_Region_setRegionName("Silicon Valley", &SiliconValley);
  DC_Region_addSchool(Scu, &SiliconValley);
  DC_College_addName("S C U", &SiliconValley.schools[0]);
  char scuNames[10][20] = {"Dennis","Ritchie","Ken","Thompson","Bjarne","Stroustrup","John","McCarthy","Haskell","Curry"};
  float scuGpas[10] = {9.5, 9.0, 10.0, 10.5, 9.6, 9.1, 80.0, 80.8, 20.0, 20.2};
  long scuIds[10] = {1000001, 1000002, 1000003, 1000004, 1000005, 1000006, 1000007, 1000008, 1000009, 1000010};
  DC_College_addStudents(scuNames, scuGpas, scuIds, &SantaClara);
  DC_Region_addSchool(SantaClara, &SiliconValley);
  DC_Region_show(&SiliconValley);


  printf("\nShowing a \"Region\" object's contained \"Student\" object & object array initialized w/ a ctor:\n");
  DC_Region_showTopStudents(&SiliconValley);

  if(jordanCR.DC_DTR){DC__NOT_Student_(&jordanCR);jordanCR.DC_DTR=NULL;}
if(koenR.DC_DTR){DC__NOT_Student_(&koenR);koenR.DC_DTR=NULL;}
if(luluR.DC_DTR){DC__NOT_Student_(&luluR);luluR.DC_DTR=NULL;}
if(charmR.DC_DTR){DC__NOT_Student_(&charmR);charmR.DC_DTR=NULL;}
if(tessaR.DC_DTR){DC__NOT_Student_(&tessaR);tessaR.DC_DTR=NULL;}
DC__Student_UDTOR_ARR(class);
DC__Student_UDTOR_ARR(group);
if(JT&&JT->DC_DTR){DC__NOT_Student_(JT);JT=NULL;}
if(Scu.DC_DTR){DC__NOT_College_(&Scu);Scu.DC_DTR=NULL;}
if(SantaClara.DC_DTR){DC__NOT_College_(&SantaClara);SantaClara.DC_DTR=NULL;}
if(willAR.DC_DTR){DC__NOT_Student_(&willAR);willAR.DC_DTR=NULL;}
if(jowiR.DC_DTR){DC__NOT_Student_(&jowiR);jowiR.DC_DTR=NULL;}
return 0;
}
