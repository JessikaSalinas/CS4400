// Grade received: 100/100

/*
 * Author: Daniel Kopta
 * Updated by: Erin Parker
 * CS 4400, University of Utah
 *
 * Simulator handout
 * A simple x86-like processor simulator.
 * Read in a binary file that encodes instructions to execute.
 * Simulate a processor by executing instructions one at a time and appropriately 
 * updating register and memory contents.
 *
 * Some code and pseudo code has been provided as a starting point.
 *
 * Completed by: Jessika Jimenez (u0864868)
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "instruction.h"

// Forward declarations for helper functions
unsigned int get_file_size(int file_descriptor);
unsigned int* load_file(int file_descriptor, unsigned int size);
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions);
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, 
				 int* registers, unsigned char* memory);
void print_instructions(instruction_t* instructions, unsigned int num_instructions);
void error_exit(const char* message);

// 17 registers
#define NUM_REGS 17
// 1024-byte stack
#define STACK_SIZE 1024

int main(int argc, char** argv)
{
  // Make sure we have enough arguments
  if(argc < 2)
    error_exit("must provide an argument specifying a binary file to execute");

  // Open the binary file
  int file_descriptor = open(argv[1], O_RDONLY);
  if (file_descriptor == -1) 
    error_exit("unable to open input file");

  // Get the size of the file
  unsigned int file_size = get_file_size(file_descriptor);
  // Make sure the file size is a multiple of 4 bytes
  // since machine code instructions are 4 bytes each
  if(file_size % 4 != 0)
    error_exit("invalid input file");

  // Load the file into memory
  // We use an unsigned int array to represent the raw bytes
  // We could use any 4-byte integer type
  unsigned int* instruction_bytes = load_file(file_descriptor, file_size);
  close(file_descriptor);

  unsigned int num_instructions = file_size / 4;

  /****************************************/
  /**** Begin code to modify/implement ****/
  /****************************************/

  // Allocate and decode instructions (left for you to fill in)
  instruction_t* instructions = decode_instructions(instruction_bytes, num_instructions);

  // Optionally print the decoded instructions for debugging
  // Will not work until you implement decode_instructions
  // Do not call this function in your submitted final version
  // print_instructions(instructions, num_instructions);

  // allocate and initialize registers
  int* registers = (int*)malloc(sizeof(int) * NUM_REGS);
  
  // initializes register values
  int i;
  for (i = 0; i < NUM_REGS; i++) {
    int stack_size = 1024;
    if (i == 6) {
      registers[i] = (int32_t)stack_size;
    }
    else {
      stack_size = 0;
      registers[i] = (int32_t)stack_size;
    }
  }

  // stack memory is byte-addressed, so it must be a 1-byte type
  unsigned char* memory = malloc(sizeof(char)*STACK_SIZE);
  
  // clears memory
  unsigned int m;
  for (m = 0; m < STACK_SIZE; m++) {
    memory[i] = 0;
  }
  
  // Run the simulation
  unsigned int program_counter = 0;

  // program_counter is a byte address, so we must multiply num_instructions by 4 
  // to get the address past the last instruction
  while(program_counter != num_instructions * 4)
  {
    program_counter = execute_instruction(program_counter, instructions, registers, memory);
  }
  
  return 0;
}

/*
 * Decodes the array of raw instruction bytes into an array of instruction_t
 * Each raw instruction is encoded as a 4-byte unsigned int
*/
instruction_t* decode_instructions(unsigned int* bytes, unsigned int num_instructions)
{
  // Allocates memory the size of num_instructions and stores in variable, retval
  instruction_t* retval = malloc(sizeof(instruction_t)*num_instructions);
  
  int i;
  for (i = 0; i < num_instructions; i++) {
    unsigned int bit_mask = 0xF8000000; // mask to assist w/ returning necessary bits from instructions
    unsigned int byte = bytes[i]; // stores the singular byte from bytes "array" at pos [i]
    int end_opcode = 27; // ending num of bits in opcode
    int end_reg1 = 22;
    int end_reg2 = 17;

    // stores 1st 5 bits as the opcode using bitmask to bitmask to return only necessary bits (31-27)
    retval[i].opcode = (byte & bit_mask)>>end_opcode;
    // reposition bitmask to just get needed bits for reg1
    bit_mask>>=5;
    // stores 2nd group of bits using the repositioned bitmask to get value of reg1 (bits 26-22)
    retval[i].first_register = (byte & bit_mask)>>end_reg1;
    // reposition again
    bit_mask>>=5;
    // stores value of reg2 (bits 21-17)
    retval[i].second_register = (byte & bit_mask)>>end_reg2;
    // change  bitmask to get the last 16 bits of instructions
    bit_mask = 0x0000FFFF;
    // stores unused and immediate bits in the instructions (16 and 15-0)
    retval[i].immediate = (byte & bit_mask);
  }
  return retval;
}


/*
 * Executes a single instruction and returns the next program counter
*/
unsigned int execute_instruction(unsigned int program_counter, instruction_t* instructions, int* registers, unsigned char* memory)
{
  // program_counter is a byte address, but instructions are 4 bytes each
  // divide by 4 to get the index into the instructions array
  instruction_t instr = instructions[program_counter / 4];
  
  // switch statements for opcodes
  switch(instr.opcode)
  { 
  // opcode 0: subl -- PASSED  
  case subl:
    registers[instr.first_register] = registers[instr.first_register] - instr.immediate;
    break;
    
  // opcode 1: addl -- PASSED
  case addl_reg_reg:
    registers[instr.second_register] = registers[instr.second_register] + registers[instr.first_register];
    break;
    
  // opcode 2: addl -- PASSED  
  case addl_imm_reg:
    registers[instr.first_register] = registers[instr.first_register] + instr.immediate;
    break;
    
  // opcode 3: imull -- PASSED                                                                         
  case imull:
    registers[instr.second_register] = registers[instr.first_register] * registers[instr.second_register];
    break;
    
  // opcode 4: shrl -- PASSED  
  case shrl:
    registers[instr.first_register] = (uint32_t)(registers[instr.first_register]) >> 1;
    break;
    
  // opcode 5: movl -- PASSED  
  case movl_reg_reg:
    registers[instr.second_register] = registers[instr.first_register];
    break;
    
  // opcode 6: movl -- PASSED
  case movl_deref_reg: ;
    int* mempointer1 = (int*)(memory + registers[instr.first_register] + instr.immediate);
    registers[instr.second_register] = mempointer1[0];
    break;
    
  // opcode 7: movl -- PASSED
  case movl_reg_deref: ;
    int* mempointer2 = (int*)(memory + registers[instr.second_register] + instr.immediate);
    mempointer2[0] = registers[instr.first_register];
    break;
    
  // opcode 8: movl -- PASSED  
  case movl_imm_reg:
    registers[instr.first_register] = instr.immediate;
    break;
     
  // opcode 9: cmpl -- PASSED
  case cmpl: ;
    // placeholders for the condition code flags
    int CF = 0;
    int ZF = 0;
    int SF = 0;
    int OF = 0;
    
    // Carry Flag
    if ((unsigned)registers[instr.first_register] > (unsigned)registers[instr.second_register]) {
      CF = 1;
    }
    // Zero Flag
    if (registers[instr.second_register] - registers[instr.first_register] == 0) {
      ZF = 64;
    }
    // Sign Flag
    if (((registers[instr.second_register] - registers[instr.first_register]) & 0x80000000) != 0) {
      SF = 128;
    }
    // Overflow Flag
    if (((registers[instr.second_register] < 0) && (registers[instr.first_register] > INT32_MAX + registers[instr.second_register])) || ((registers[instr.second_register] > 0) && (registers[instr.first_register] < INT32_MIN + registers[instr.second_register]))) {
      OF = 2048;
    }
    
    registers[16] = CF + ZF + SF + OF;
    break;
    
  // opcode 10: je -- PASSED
  case je:
      // check AND with 64 (ie ZF)
      if ((registers[16] & 0x00000040) != 0) {
	return program_counter + 4 + (instr.immediate);
      }
    break;
    
  // opcode 11: jl -- PASSED
  case jl:
      // check AND with 2048 (ie OF) and AND with 128 (ie SF) then XOR them
      if (((registers[16] & 0x00000800) != 0) ^ (registers[16] & 0x00000080) != 0)  {
        return program_counter + 4 + (instr.immediate);
      }
    break;
    
  // opcode 12: jle -- PASSED
  case jle:
      // check AND with 2048 (ie OF) and AND with 128 (ie SF) then XOR them
      // then check AND with 64 (ie ZF) and OR them
      if ((((registers[16] & 0x00000800) != 0) ^ (registers[16] & 0x00000080) != 0) || (registers[16] & 0x00000040) != 0)  {
        return program_counter + 4 + (instr.immediate);
      }
    break;
    
  // opcode 13: jge -- PASSED
  case jge:
      // check AND with 2048 (ie OF) and AND with 128 (ie SF) then not-XOR them
      if (!(((registers[16] & 0x00000800) != 0) ^ (registers[16] & 0x00000080) != 0))  {
        return program_counter + 4 + (instr.immediate);
      }
    break;
    
  // opcode 14: jbe -- PASSED
  case jbe:
      // check AND with 64 (ie ZF) and AND with 1 (ie CF), then OR them
      if (((registers[16] & 0x00000040) != 0) || (registers[16] & 0x00000001) != 0)  {
        return program_counter + 4 + (instr.immediate);
      }
    break;
    
  // opcode 15: jmp -- PASSED  
  case jmp:
      // unconditional jump
      return program_counter + 4 + (instr.immediate);
    break;
    
  // opcode 16: call -- PASSED 
  case call: 
      registers[6] = registers[6] - 4;
      int* mempointer3 = (int*)(memory + registers[6]);
      mempointer3[0] = program_counter + 4;
      return program_counter + 4 + (instr.immediate);
    break;
    
  // opcode 17: ret -- PASSED  
  case ret:
      if (registers[6] == 1024) {
	exit(0);
      }
      else {
        int* mempointer4 = (int*)(memory + registers[6]);
      program_counter = mempointer4[0];
	registers[6] = registers[6] + 4;
	return program_counter;
      }
    break;
    
  // opcode 18: pushl -- PASSED
  case pushl:
      registers[6] = registers[6] - 4;
        int* mempointer5 = (int*)(memory + registers[6]);
	mempointer5[0] = registers[instr.first_register];
    break;
    
  // opcode 19: popl -- PASSED
  case popl: ;
      int* mempointer6 = (int*)(memory + registers[6]);
      registers[instr.first_register] = mempointer6[0];
      registers[6] = registers[6] + 4;
    break;
    
  // opcode 20: printr -- PASSED 
  case printr:
    printf("%d (0x%x)\n", registers[instr.first_register], registers[instr.first_register]);
    break;
    
  // opcode 21: readr -- PASSED
  case readr: ;
    scanf("%d", &(registers[instr.first_register]));
    break;

  default:
    break;
  }

  // program_counter + 4 represents the subsequent instruction
  return program_counter + 4;
}


/*********************************************/
/****  DO NOT MODIFY THE FUNCTIONS BELOW  ****/
/*********************************************//*
 * Returns the file size in bytes of the file referred to by the given descriptor
*/
unsigned int get_file_size(int file_descriptor)
{
  struct stat file_stat;
  fstat(file_descriptor, &file_stat);
  return file_stat.st_size;
}

/*
 * Loads the raw bytes of a file into an array of 4-byte units
*/
unsigned int* load_file(int file_descriptor, unsigned int size)
{
  unsigned int* raw_instruction_bytes = (unsigned int*)malloc(size);
  if(raw_instruction_bytes == NULL)
    error_exit("unable to allocate memory for instruction bytes (something went really wrong)");

  int num_read = read(file_descriptor, raw_instruction_bytes, size);

  if(num_read != size)
    error_exit("unable to read file (something went really wrong)");

  return raw_instruction_bytes;
}

/*
 * Prints the opcode, register IDs, and immediate of every instruction, 
 * assuming they have been decoded into the instructions array
*/
void print_instructions(instruction_t* instructions, unsigned int num_instructions)
{
  printf("instructions: \n");
  unsigned int i;
  for(i = 0; i < num_instructions; i++)
  {
    printf("op: %d, reg1: %d, reg2: %d, imm: %d\n", 
	   instructions[i].opcode,
	   instructions[i].first_register,
	   instructions[i].second_register,
	   instructions[i].immediate);
  }
  printf("--------------\n");
}

/*
 * Prints an error and then exits the program with status 1
*/
void error_exit(const char* message)
{
  printf("Error: %s\n", message);
  exit(1);
}
