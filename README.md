# SYSC 4001 Assignment 3  
## Part 2: Concurrent Processes in Unix  
### Student 1: Ismael Ouadria 101284947
### Student 2: Aziz Hamad 101232108  
### Source File: part2_101284947_101232108.cpp  
### Executable: a3p2

---

## 1. Overview

This program implements Part 2(a) and Part 2(b) of Assignment 3. It simulates multiple Teaching Assistants marking exams concurrently. Each TA is a separate process created with ```fork()```. All TAs share a rubric and the current exam through POSIX shared memory. Semaphores coordinate access to the rubric, marking, and loading the next exam. The program loads exams in sequence and stops when it reaches the student number 9999.

---

## 2. Authors

Student 1: Ismael Ouadria 101284947  
Student 2: Aziz Hamad 101232108

---

## 3. Directory Structure

```
SYSC4001_A3P2/
 ├── part2_101284947_101232108.cpp
 ├── a3p2
 ├── rubric.txt
 └── exams/
      ├── 0001.txt
      ├── 0002.txt
      ├── ...
      └── 9999.txt
```

The exams folder must contain at least twenty exam files. The file 9999.txt ends the execution. For the sake of this assignment, the numbering is from 0001.txt to 0019.txt, followed by 9999.txt as the 20th file.

---

## 4. Compilation

### macOS (clang++)
```
clang++ -std=c++17 part2_101284947_101232108.cpp -o a3p2 -pthread
```

### Linux (g++)
```
g++ -std=c++17 part2_101284947_101232108.cpp -o a3p2 -pthread
```

---

## 5. Running the Program

Run the executable with the number of TAs. The minimum is 2.

Examples:

```
./a3p2 2
```

```
./a3p2 3
```

```
./a3p2 5
```

---

## 6. Test Cases

The test cases for Part 2 are the exam files in the exams folder. Each file contains a student number on the first line. The program loads exams in numeric order. When the student number is 9999, all TAs stop.

---

## 7. Expected Output

During execution the program prints:

- When each TA starts  
- When a TA reviews the rubric  
- If a rubric entry is kept or changed  
- When a TA marks a question  
- When a TA loads the next exam  
- When the final exam is reached  

Example:

```
TA 1 started correcting exams. Starting with student 1
TA 2 is reviewing rubric for student 1
TA 2 changed the rubric answer for Q1 from A to B
TA 1 marked Q1 for student 1
TA 2 loaded the next exam. Correcting the exam for student 2
TA 1 is ending the exam correcting. Final exam reached.
```
