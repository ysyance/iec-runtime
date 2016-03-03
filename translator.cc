#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>


using namespace std;
// OBJ Header
#define MAGIC "\x7fPLC\0"
#define SYS_TYPE_32  1
#define SYS_TYPE_64  2
#define BYTE_ORDER_LIT  1
#define BYTE_ORDER_BIT  2
#define MACH_CORTEX_A8 1
// OBJ Servo Configuration Segment
#define AXIS_TYPE_FINITE  1
#define AXIS_TYPE_MODULO  2
#define AXIS_INDEPENDENT  1
#define AXIS_COMBINED  2
#define OPMODE_POS  1
#define OPMODE_VEL  2
#define OPMODE_TOR  3
// OBJ PLC Task Segment
#define TASK_TYPE_SIGNAL  1
#define TASK_TYPE_INTERVAL  2
// OBJ PLC Task Constant/Global Value Type
#define TINT  1
#define TUINT  2
#define TDOUBLE  3
#define TSTRING  4




// VM Instruction Encoding (MUST be equal to iec-runtime)
#define SIZE_OP  8
#define SIZE_A   8
#define SIZE_B   8
#define SIZE_C   8
#define SIZE_Bx  (SIZE_B+SIZE_C)
#define SIZE_sAx (SIZE_A+SIZE_B+SIZE_C)

#define POS_C   0
#define POS_B   (POS_C+SIZE_C)
#define POS_A   (POS_B+SIZE_B)
#define POS_OP  (POS_A+SIZE_A)
#define POS_Bx  POS_C
#define POS_sAx POS_C

#define BIAS_sAx (1<<(SIZE_sAx-1))


//  Original Instruction Encoder
#define create_ABC(operand) 	(opcode[operand[1]] << POS_OP \
            						| std::stoi(operand[2]) << POS_A \
            						| std::stoi(operand[3]) << POS_B \
            						| std::stoi(operand[4]) << POS_C)

#define create_ABx(operand)     (opcode[operand[1]] << POS_OP \
            						| std::stoi(operand[2]) << POS_A \
            						| std::stoi(operand[3]) << POS_Bx)

#define create_sAx(operand)     (opcode[operand[1]] << POS_OP \
            						| std::stoi(operand[2])+BIAS_sAx << POS_sAx)


// data move
#define OP_GLOAD  1 
#define OP_GSTORE  2 
#define OP_KLOAD  3 
#define OP_LDLOAD  4 
#define OP_LDSTORE  5 
#define OP_LALOAD  6 
#define OP_LASTORE  7 
#define OP_RDLOAD  8 
#define OP_RDSTORE  9 
#define OP_RALOAD  10 
#define OP_RASTORE  11 
#define OP_MOV  12 
// arithmetic
#define OP_ADD  13 
#define OP_SUB  14 
#define OP_MUL  15 
#define OP_DIV  16 
// bit operation
#define OP_SHL  17 
#define OP_SHR  18 
#define OP_AND  19 
#define OP_OR  20 
#define OP_XOR  21 
#define OP_NOT  22 
// logic operation
#define OP_LAND  23 
#define OP_LOR  24 
#define OP_LXOR  25 
#define OP_LNOT  26 
// comparison
#define OP_LT  27 
#define OP_LE  28 
#define OP_GT  29 
#define OP_GE  30 
#define OP_EQ  31
#define OP_NE  32 
// flow control
#define OP_CONDJ  33 
#define OP_JMP  34 
#define OP_HALT  35 
// call
#define OP_SCALL  36 
#define OP_UCALL  37 
#define OP_RET  38


std::map<std::string, int> opcode = { 
								// data move
								{"OP_GLOAD", 1},
								 {"OP_GSTORE", 2},
								 {"OP_KLOAD", 3},
								 {"OP_LDLOAD", 4},
								 {"OP_LDSTORE", 5},
								 {"OP_LALOAD", 6},
								 {"OP_LASTORE", 7},
								 {"OP_RDLOAD", 8},
								 {"OP_RDSTORE", 9},
								 {"OP_RALOAD", 10},
								 {"OP_RASTORE", 11},
								 {"OP_MOV", 12},
								 // arithmetic
								 {"OP_ADD", 13},
								 {"OP_SUB", 14},
								 {"OP_MUL", 15},
								 {"OP_DIV", 16},
								 // bit operation
								 {"OP_SHL", 17},
								 {"OP_SHR", 18},
								 {"OP_AND", 19},
								 {"OP_OR", 20},
								 {"OP_XOR", 21},
								 {"OP_NOT", 22},
								 // logic operation
								 {"OP_LAND", 23},
								 {"OP_LOR", 24},
								 {"OP_LXOR", 25},
								 {"OP_LNOT", 26},
								 // comparison
								 {"OP_LT", 27},
								 {"OP_LE", 28},
								 {"OP_GT", 29},
								 {"OP_GE", 30},
								 {"OP_EQ", 31},
								 {"OP_NE", 32},
								 // flow control
								 {"OP_CONDJ", 33},
								 {"OP_JMP", 34},
								 {"OP_HALT", 35},
								 // call
								 {"OP_SCALL", 36},
								 {"OP_UCALL", 37},
								 {"OP_RET", 38},
								 // helper
        						 {"OP_LDIX", 4},
        						 {"OP_LDIB", 4},
        						 {"OP_LDIW", 4},
        						 {"OP_LDID", 4},
        						 {"OP_LDOX", 5},
        						 {"OP_LDOB", 5},
        						 {"OP_LDOW", 5},
        						 {"OP_LDOD", 5},
        						 {"OP_RDIX", 8},
        						 {"OP_RDIB", 8},
        						 {"OP_RDIW", 8},
        						 {"OP_RDID", 8},
        						 {"OP_RDOX", 9},
        						 {"OP_RDOB", 9},
        						 {"OP_RDOW", 9},
        						 {"OP_RDOD", 9}

								};
//字符串分割函数
std::vector<std::string> split(std::string str,std::string pattern)
{
  std::string::size_type pos;
  std::vector<std::string> result;
  std::vector<std::string> realresult;
  str+=pattern;//扩展字符串以方便操作
  int size=str.size();
 
  for(int i=0; i<size; i++)
  {
    pos=str.find(pattern,i);
    if(pos<size)
    {
      std::string s=str.substr(i,pos-i);
      result.push_back(s);
      i=pos+pattern.size()-1;
    }
  }
  for(auto elem : result) {
  	if(!elem.empty()){
  		realresult.push_back(elem);
  	}
  }
  return realresult;
}


#define gen_debug_info(step,info) std::cout << "step " << step << ": " << info << std::endl

int main(int argc, char const *argv[])
{
	std::ifstream infile;
	std::ofstream outfile;
	std::string strline;
	std::vector<std::string> result;
	int temp = 0;

	if(argc < 2) {
		std::cout << "lack of arguments" << std::endl;
		return -1;
	}
	std::cout << "generating object file..." << std::endl;
	infile.open(argv[1]);
	outfile.open("exe.obj", std::ofstream::binary);
	while(!infile.eof()) {
		getline(infile, strline);
		result = split(strline, " ");
#ifdef DEBUG
		std::cout << result.size() << ": ";
		for(auto elem : result) {
			std::cout << elem << " ";
		}
#endif
		if(result.size() > 0) {
			if(result[0] == "header_magic") {
				outfile << MAGIC << '\0';
				gen_debug_info(1, "header_magic");
			} else if(result[0] == "header_type") {
				outfile << (char)(result[1] == "SYS_TYPE_32" ? SYS_TYPE_32 : SYS_TYPE_64);
				gen_debug_info(2, "header_type");
			}else if(result[0] == "header_order") {
				outfile << (char)(result[1] == "BYTE_ORDER_LIT" ? BYTE_ORDER_LIT : BYTE_ORDER_BIT);
				gen_debug_info(3, "header_order");
			} else if(result[0] == "header_version") {
				outfile << (char)1;
				gen_debug_info(4, "header_version");
			} else if(result[0] == "header_machine") {
				outfile << (char)MACH_CORTEX_A8;
				gen_debug_info(5, "header_machine");
			} else if(result[0] == "iocs_update_interval") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 4);
				gen_debug_info(6, "iocs_update_interval");
			} else if(result[0] == "iocs_ldi_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				// outfile << (char)std::stoi(result[1]);
				gen_debug_info(7, "iocs_ldi_count");
			} else if(result[0] == "iocs_ldo_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(8, "iocs_ldo_count");
			} else if(result[0] == "iocs_lai_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(9, "iocs_lai_count");
			} else if(result[0] == "iocs_lao_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(10, "iocs_lao_count");
			} else if(result[0] == "iocs_rdi_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(11, "iocs_rdi_count");
			} else if(result[0] == "iocs_rdo_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(12, "iocs_rdo_count");
			} else if(result[0] == "iocs_rai_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(13, "iocs_rai_count");
			} else if(result[0] == "iocs_rao_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(14, "iocs_rao_count");
			} else if(result[0] == "scs_axis_count") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(15, "scs_axis_count");
			} else if(result[0] == "scs_update_interval") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 4);
				gen_debug_info(16, "scs_update_interval");
			} else if(result[0] == "acs_name") {
				outfile << setw(16) << setfill('\0') << setiosflags(ios::left) << result[1];
				gen_debug_info(17, "acs_name");
			} else if(result[0] == "acs_id") {
				temp = std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(18, "acs_id");
			} else if(result[0] == "acs_type") {
				outfile << (char)(result[1] == "AXIS_TYPE_FINITE" ? AXIS_TYPE_FINITE : AXIS_TYPE_MODULO);
				gen_debug_info(19, "acs_type");
			} else if(result[0] == "acs_combined") {
				outfile << (char)(result[1] == "AXIS_INDEPENDENT" ? AXIS_INDEPENDENT : AXIS_COMBINED);
				gen_debug_info(20, "acs_combined");
			} else if(result[0] == "acs_opmode") {
				outfile << (char)(result[1] == "OPMODE_POS" ? OPMODE_POS : (result[1] == "OPMODE_VEL" ? OPMODE_VEL:OPMODE_TOR));
				gen_debug_info(21, "acs_opmode");
			} else if(result[0] == "acs_min_pos") {
				double tmp = std::stod(result[1]);
				outfile.write((char*)&tmp, 8);
				gen_debug_info(22, "acs_min_pos");
			} else if(result[0] == "acs_max_pos") {
				double tmp = std::stod(result[1]);
				outfile.write((char*)&tmp, 8);
				gen_debug_info(23, "acs_max_pos");
			} else if(result[0] == "acs_max_vel") {
				double tmp = std::stod(result[1]);
				outfile.write((char*)&tmp, 8);
				gen_debug_info(24, "acs_max_vel");
			} else if(result[0] == "acs_max_acc") {
				double tmp = std::stod(result[1]);
				outfile.write((char*)&tmp, 8);
				gen_debug_info(25, "acs_max_acc");
			} else if(result[0] == "acs_max_dec") {
				double tmp = std::stod(result[1]);
				outfile.write((char*)&tmp, 8);
				gen_debug_info(26, "acs_max_dec");
			} else if(result[0] == "acs_max_jerk") {
				double tmp = std::stod(result[1]);
				outfile.write((char*)&tmp, 8);
				gen_debug_info(27, "acs_max_jerk");
			} else if(result[0] == "plc_task_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(28, "plc_task_count");
			} else if(result[0] == "tds_name") {
				outfile << setw(16) << setfill('\0') << setiosflags(ios::left) << result[1];
				gen_debug_info(29, "tds_name");
			} else if(result[0] == "tds_priority") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(30, "tds_priority");
			} else if(result[0] == "tds_type") {
				outfile << (char)(result[1] == "TASK_TYPE_INTERVAL" ? TASK_TYPE_INTERVAL : TASK_TYPE_SIGNAL);
				gen_debug_info(31, "tds_type");
			} else if(result[0] == "tds_signal") {
				outfile << (char)std::stoi(result[1]);
				gen_debug_info(32, "tds_signal");
			} else if(result[0] == "tds_interval") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 4);
				gen_debug_info(33, "tds_interval");
			} else if(result[0] == "tds_sp_size") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 4);
				gen_debug_info(34, "tds_sp_size");
			} else if(result[0] == "tds_cs_size") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 2);
				gen_debug_info(35, "tds_cs_size");
			} else if(result[0] == "tds_pou_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 2);
				gen_debug_info(36, "tds_pou_count");
			} else if(result[0] == "tds_const_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 2);
				gen_debug_info(37, "tds_const_count");
			} else if(result[0] == "tds_global_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 2);
				gen_debug_info(38, "tds_global_count");
			} else if(result[0] == "tds_inst_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 4);
				gen_debug_info(39, "tds_inst_count");
			} else if(result[0] == "pds_name") {
				outfile << setw(20) << setfill('\0') << setiosflags(ios::left) << result[1];
				gen_debug_info(40, "pds_name");
			} else if(result[0] == "pds_input_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(41, "pds_input_count");
			} else if(result[0] == "pds_inout_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(42, "pds_inout_count");
			} else if(result[0] == "pds_output_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(43, "pds_output_count");
			} else if(result[0] == "pds_local_count") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 1);
				gen_debug_info(44, "pds_local_count");
			} else if(result[0] == "pds_entry") {
				temp = (int)std::stoi(result[1]);
				outfile.write((char*)&temp, 4);
				gen_debug_info(45, "pds_entry");
			} else if(result[0] == "K") {
				std::cout << "Constant variables" << std::endl;
				if(result[1] == "TUINT") {
					outfile << (char)TUINT;
					unsigned long tmp = std::stoul(result[2]);
					outfile.write((char*)&tmp, 8);
				} else if(result[1] == "TINT") {
					outfile << (char)TINT;
					long tmp = std::stol(result[2]);
					outfile.write((char*)&tmp, 8);
				} else if(result[1] == "TDOUBLE") {
					outfile << (char)TDOUBLE;
					double tmp = std::stod(result[2]);
					outfile.write((char*)&tmp, 8);
				} else if(result[1] == "TSTRING") {
					outfile << (char)TSTRING;
				}
			} else if(result[0] == "G") {
				std::cout << "Global variables" << std::endl;
				if(result[1] == "TUINT") {
					outfile << (char)TUINT;
					unsigned long tmp = std::stoul(result[2]);
					outfile.write((char*)&tmp, 8);
				} else if(result[1] == "TINT") {
					outfile << (char)TINT;
					long tmp = std::stol(result[2]);
					outfile.write((char*)&tmp, 8);
				} else if(result[1] == "TDOUBLE") {
					outfile << (char)TDOUBLE;
					double tmp = std::stod(result[2]);
					outfile.write((char*)&tmp, 8);
				} else if(result[1] == "TSTRING") {
					outfile << (char)TSTRING;
				}
			} else if(result[0] == "I") {
				std::cout << "Instruction" << std::endl;
				std::cout << result[1] << ": " << opcode[result[1]] << std::endl;
				switch(opcode[result[1]]) {
					// data move
					case OP_GLOAD: 
						temp = create_ABx(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_GSTORE:
						temp = create_ABx(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_KLOAD:
						temp = create_ABx(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_LDLOAD:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_LDSTORE:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_LALOAD:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_LASTORE:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_RDLOAD:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_RDSTORE:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_RALOAD:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_RASTORE: 
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_MOV: 
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					// arithmetic
					case OP_ADD:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_SUB:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_MUL: 
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_DIV: 
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					// bit operation
					case OP_SHL:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_SHR:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_AND:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_OR:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_XOR: 
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_NOT:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					// logic operation
					case OP_LAND:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break; 
					case OP_LOR:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_LXOR: 
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_LNOT:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					// comparison
					case OP_LT:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_LE:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_GT:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_GE:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_EQ:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_NE:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					// flow control
					case OP_CONDJ:
						temp = create_ABx(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_JMP:
						temp = create_sAx(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_HALT:
						temp = create_ABC(result);
						outfile.write((char*)&temp,4);
						break;
					// call
					case OP_SCALL:
						break;
					case OP_UCALL:
						temp = create_ABx(result);
						outfile.write((char*)&temp,4);
						break;
					case OP_RET:
						temp = create_ABx(result);
						outfile.write((char*)&temp,4);
						break;
				}
			}
		}
		
	}
	infile.close();
	outfile.close();

	return 0;
}



























