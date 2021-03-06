#include "Register.h"
#include "stdHeader.h"

//////////////////////////////////////
//
//  Constructor for the Register file.
//
//////////////////////////////////////
Register::Register(std::vector<u32> reg)
{
  reg1 = 0;
  reg2 = 0;
  registers = reg;
  write_reg = 0;
  write_data = 0;
  control_write = 0;
}

//////////////////////////////////////
//  Destructor for the Register file.
//////////////////////////////////////
Register::~Register() {}

//////////////////////////////////////
//  Destructor for the Register file.
//////////////////////////////////////
Register::Register() {}

//////////////////////////////////////
//  Write back method for the data
// memory block.
//////////////////////////////////////
void Register::write()
{
  registers[write_reg] = write_data;
}

///////////////////////////////////
//
//  Print out all the control lines
//  and the opcode.
//
//////////////////////////////////
void Register::print_out()
{
  std::cout << " --------------- " << std::endl;
  std::cout << "| Register File |" << std::endl;
  std::cout << " --------------- " << std::endl;
  std::cout << "Register 1: " << reg1 << std::endl;
  std::cout << "Register 2: " << reg2 << std::endl;
  std::cout << "Write Register: " << write_reg << std::endl;
  printf("Write Data: 0x%08x\n", write_data);
  std::cout << "Register Contents..." << std::endl;
  for(int i=0; i < 32; i++)
  {
    printf("%d: 0x%08x\n", i, registers.at(i));
  }

  std::cout << std::endl;

}
