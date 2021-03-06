#include "Parser.h"

// -----
//  Parser
//	  Constructs a Parser instance based on a specific input file
//		and reads in the files named in the config file.
//
//	PARAMS
//		filename - a string of the filename of the configuration
//						file to be used.
Parser::Parser(std::string filename)
{
	read_config_file(filename);

	register_file.resize(32);

	read_memory_contents();
	read_register_file();
	read_program();
}

// -----
//  ~Parser
//	  Destructs a Parser instance.
Parser::~Parser()
{
}

// Define an enumeration of the opcodes that will be
// 6-bit opcode field and 6-bit func field set
enum Opcode
{
	UNDEF = 0,
	ADD = 	(0x00 << 26) | (32),
	SUB = 	(0x00 << 26) | (34),
	ADDI = 	(0x08 << 26) | (0),
	SLT = 	(0x00 << 26) | (42),
	LW = 	(0x23 << 26) | (0),
	SW = 	(0x2b << 26) | (0),
	BEQ =	(0x04 << 26) | (0),
	J = 	(0x02 << 26) | (0)
};

// -----
//  get_register
//	  Reads a decimal value from a string if a '$' is present, meaning it
//		is a register. Otherwise, reads as hex and sets a flag.
u32 get_register(char *f)
{
	for(unsigned int i = 0; i < strlen(f); ++i)
		if(f[i] == '$')
			return strtoul(f + 1, NULL, 10);

	// Set a flag in MSB for a value returned that did not contain
	// a '$' -- meaning the value is actually a shift amount
	return strtoul(f, NULL, 16) | 0x80000000;
}

// -----
//  handle_RType
//	  Returns the three 5-bit registers and the shift amount between
//			the Opcode and func fields of a MIPS instruction.
u32 handle_RType(char *fields)
{
	char *rd, *rs, *rt;
	int rd_n = 0, rs_n, rt_n, sh_n = 0;

	// All R-Type instructions we are handling will have 3 register fields
	if((rd = strtok(fields, ", ")) == NULL)
		return(0);

	if((rs = strtok(NULL, ", ")) == NULL)
		return(0);

	if((rt = strtok(NULL, ", ")) == NULL)
		return(0);

	u32 t = get_register(rd);

	// Determine whether the 'this is a shift amount' flag is set or not
	if(t >> 31)
		sh_n = t & 0x7FFFFFFF;
	else
		rd_n = t;

	rs_n = get_register(rs);
	rt_n = get_register(rt);

	return (rs_n << 21)
		 | (rt_n << 16)
		 | (rd_n << 11)
		 | (sh_n <<  6);
}

// -----
//  handle_IType
//	  Returns the two, 5-bit register values in proper order and
//	  	the 16-bit immediate field; all in a 32-bit word.
u32 handle_IType(char *fields)
{
	char *rs, *rt, *imm;
	bool offset = true;

	int rs_n, rt_n, imm_n;

	// Check if we have the immediate field 2nd (true) or 3rd (false)
	int commas = 0;

	for(unsigned int i = 0; i < strlen(fields); ++i)
		if(fields[i] == ',')
			commas++;

	if(commas == 2)
		offset = false;

	if((rt = strtok(fields, ", ")) == NULL)
		return(0);

	rt_n = get_register(rt);

	if(!offset)	// $rt, $rs, imm
	{
		if((rs = strtok(NULL, ", ")) == NULL)
			return(0);

		if((imm = strtok(NULL, ", ")) == NULL)
			return(0);

		rs_n = get_register(rs);

		// The offset (immediate) value in ADDI can be expressed either
		// in hexadecimal or decimal, strtol with 0 to determine which
		imm_n = (u16) strtol(imm, NULL, 0);
	}
	else	// $rt, imm($rs) -> $rt
	{
		if((imm = strtok(NULL, ",()")) == NULL)	// imm($rs) -> imm
			return(0);

		if((rs = strtok(NULL, ",()")) == NULL)	// $rs) -> $rs
			return(0);

		rs_n = get_register(rs);

		// The offset field in LW, and SW instructions is represented in
		// decimal, and may be either positive or negative
		imm_n = (u16) strtol(imm, NULL, 10);
	}

	return (rs_n << 21)
		 | (rt_n << 16)
		 | (imm_n);
}

// -----
//  handle_JType
//	  Returns the formatted 26-bit address field of a J-Type MIPS instruction.
u32 handle_JType(char *fields)
{
	// Fill last 26 bits with value
	return 0x03FFFFFF & (strtoul(fields, NULL, 16) >> 2);
}

// -----
//  match_case
//	  Returns whether two words are equal to eachother, case insensitive.
//
//	PARAMS
//		a - a null-terminated byte string which we'll check is equal to b
//		b - a null-terminated byte string which we'll check is equal to a
//
//	RETURN
//		true if the strings are lexicographically, but not integrally, equal
bool match_case(const char *a, const char *b)
{
	int a_len = strlen(a);

	bool result = true;

	// Check if the char values are equal, or 
	// just the same letter regardless of case
	for(int i = 0; i < a_len; ++i)
		result &= (a[i] - b[i] == 0
				|| a[i] - b[i] == ('a' - 'A')
				|| a[i] - b[i] == ('A' - 'a'));

	return result;
}

// -----
//  translate_to_machine
//	  Translates each of the MIPS instruction lines in
// 	  the instruction vector field string_instructions
//	  		into 32-bit machine code.
//
//	PARAMS
//		line - an instruction line to convert
//
//	RETURN
//		the MIPS assembly instruction's machine code
//			representation
u32 Parser::translate_to_machine(std::string line)
{
	u32 instruction = 0;

	// Take the line as a null-terminated byte string
	char *buf = strdup(line.c_str());
	char *buf_s = buf; 	// Actual string start so we can free

	// Replace tabs with spaces to make parsing easier
	for(unsigned int i = 0; i < strlen(buf); ++i)
		if(buf[i] == '\t')
			buf[i] = ' ';

	// Get the Opcode from the instruction line
	char *opcode = strtok(buf, " ");
	char *fields = strtok(NULL, "");	// Don't null the next delimiter

	Opcode op = UNDEF;

	if(match_case("add", opcode))	op = ADD;
	if(match_case("sub", opcode))	op = SUB;
	if(match_case("addi", opcode))	op = ADDI;
	if(match_case("slt", opcode))	op = SLT;
	if(match_case("lw", opcode))	op = LW;
	if(match_case("sw", opcode))	op = SW;
	if(match_case("beq", opcode))	op = BEQ;
	if(match_case("j", opcode))		op = J;

	// Handle the fields we'd read in differently based upon the type of instruction
	switch(op)
	{
		case ADD:
			instruction |= ADD;
			instruction |= handle_RType(fields);
			break;
		case SUB:
			instruction |= SUB;
			instruction |= handle_RType(fields);
			break;
		case ADDI:
			instruction |= ADDI;
			instruction |= handle_IType(fields);
			break;
		case SLT:
			instruction |= SLT;
			instruction |= handle_RType(fields);
			break;
		case LW:
			instruction |= LW;
			instruction |= handle_IType(fields);
			break;
		case SW:
			instruction |= SW;
			instruction |= handle_IType(fields);
			break;
		case BEQ:
			instruction |= BEQ;
			instruction |= handle_IType(fields);
			break;
		case J:
			instruction |= J;
			instruction |= handle_JType(fields);
			break;
		default:
			free(buf_s);
			return(0);
	}

	free(buf_s);

	return instruction;
}

// -----
//  stripLine
//	  Strips comments and any trailing or leading
//	  	whitespace from an input line.
//
//	PARAMS
//		line - an input line to strip
//
//	RETURN
//		a string with no trailing/leading whitespace
//				or comments
std::string stripLine(std::string line)
{
	int size = line.size();

	char *buf = strdup(line.c_str());	// Duplicated const char *
	char *buf_s = buf; 	// Actual string start so we can free

	// Cut off any comments
	for(int i = 0; i < size; ++i)
		if(buf[i] == '#')
			buf[i] = '\0';

	// Strip any leading whitespace
	while(buf[0] != '\0' && (buf[0] == ' ' || buf[0] == '\t'))
		buf++;

	int end = strlen(buf) - 1;

	// If nothing remains, ignore the line
	if(end < 0)
		return "";

	// Strip any trailing whitespace
	while(end != 0 && buf[end] != '\0'
			&& (buf[end] == ' ' || buf[end] == '\t'))
	{
		buf[end] = '\0';
		end--;
	}

	// Replace tabs with spaces to make parsing easier
	for(unsigned int i = 0; i < strlen(buf); ++i)
		if(buf[i] == '\t')
			buf[i] = ' ';

	// Store the line
	std::string temp = buf;

	// Free all of the allocated memory from the original start
	free(buf_s);

	return temp;
}

// -----
//  Parser::read_register_file
//	  Reads in and parses lines from the register file input
//		containing <register:value> pairs.
void Parser::read_register_file()
{
	std::ifstream input;

	input.open(register_file_input.c_str());

	if(input.bad() || !input)
	{
		printf("Could not open file \"%s\"!\n", register_file_input.c_str());
	}
	else
	{
		std::string line;
		int lineNum = 0;

		while(getline(input, line))
		{
			lineNum++;

			std::string str = stripLine(line);

			if(str.size() == 0)
				continue;

			char *buf = strdup(str.c_str());
			char *buf_s = buf;

			char *reg;
			char *value;

			reg = strtok(buf, ":");

			if((value = strtok(NULL,":")) == NULL)
			{
				printf("Malformed input on line %d!\n", lineNum);
				free(buf_s);
				break;
			}
			else
			{
				// Take chars and convert each into its int value;
				// decimal for register number and hex for the value
				u32 r = std::strtoul(reg, NULL, 10);
				u32 val = std::strtoul(value, NULL, 16);

				// Ignore invalid-valued registers
				if(r > 31)
					continue;

				register_file[r] = val;
			}

			free(buf_s);
		}
	}
}

// -----
//  Parser::read_memory_contents
//	  Reads in and parses lines from the initial memory contents
//		file containing <address:value> pairs.
void Parser::read_memory_contents()
{
	std::ifstream input;

	input.open(memory_contents_input.c_str());

	if(input.bad() || !input)
	{
		printf("Could not open file \"%s\"!\n", memory_contents_input.c_str());
	}
	else
	{
		std::string line;
		int lineNum = 0;

		while(getline(input, line))
		{
			lineNum++;

			std::string str = stripLine(line);

			if(str.size() == 0)
				continue;

			char *buf = strdup(str.c_str());
			char *buf_s = buf;

			char *address;
			char *value;

			address = strtok(buf, ":");

			if((value = strtok(NULL,":")) == NULL)
			{
				printf("Malformed input on line %d!\n", lineNum);
				free(buf_s);
				break;
			}
			else
			{
				// Take string and convert it into a hexadecimal number
				u32 addr = std::strtoul(address, NULL, 16);
				u32 val = std::strtoul(value, NULL, 16);

				memory_module[addr] = val;
			}

			free(buf_s);
		}
	}
}

// -----
//  Parser::read_program
//	  Reads in and parses lines from the input program file
//		and stores them in the member vector field.
void Parser::read_program()
{
	std::ifstream input;

	input.open(program_input.c_str());

	if(input.bad() || !input)
	{
		printf("Could not open file \"%s\"!\n", program_input.c_str());
	}
	else
	{
		std::string line;
		int lineNum = 0;

		while(getline(input, line))
		{

			std::string str = stripLine(line);

			if(str.size() == 0)
				continue;

			// Store the line in the instruction vector
			string_instructions.push_back(str);

			lineNum++;

			// Convert the line to machine code and store
			u32 instruction = translate_to_machine(str);

			instruction_memory.push_back(instruction);
		}
		
		instruction_mem_size = lineNum;
	}
}

// -----
//  Parser::read_config_file
//	  Reads in and parses lines from the input configuration
//		file and assigns the values to Parser's fields.
//
//	PARAMS
//		filename - string name of the input config file
void Parser::read_config_file(std::string filename)
{
	std::ifstream input;

	input.open(filename.c_str());

	if(input.bad() || !input)
	{
		printf("Could not open file \"%s\"!\n", filename.c_str());
	}
	else
	{
		std::string line;
		int lineNum = 0;

		while(getline(input, line))
		{
			lineNum++;

			std::string str = stripLine(line);

			if(str.size() == 0)
				continue;

			char *buf = strdup(str.c_str());
			char *buf_s = buf;

			char *parameter;
			char *value;

			parameter = strtok(buf, "=");

			if((value = strtok(NULL,"=")) == NULL)
			{
				printf("Malformed input on line %d!\n", lineNum);
				free(buf_s);
				break;
			}
			else
			{
				if (!strcmp(parameter, "program_input"))
					program_input = value;
				else if (!strcmp(parameter, "memory_contents_input"))
					memory_contents_input = value;
				else if (!strcmp(parameter, "register_file_input"))
					register_file_input = value;
				else if (!strcmp(parameter, "output_mode"))
					output_mode = value;
				else if (!strcmp(parameter, "debug_mode"))
					debug_mode = value;
				else if (!strcmp(parameter, "print_memory_contents"))
					print_memory_contents = value;
				else if (!strcmp(parameter, "output_file"))
					output_file = value;
				else if (!strcmp(parameter, "write_to_file"))
					write_to_file = value;
				else
				{
					printf("Malformed input on line %d!\n", lineNum);
					free(buf_s);
					break;
				}
			}

			free(buf_s);
		}
	}
}
