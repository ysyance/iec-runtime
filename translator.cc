#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <vector>
#include <functional>



#define DEBUG   //for debug

#define gen_debug_info(step,info) std::cout << "step " << step << ": " << info << std::endl

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

typedef struct {
	int bytes;
	std::string type;
	bool is_macro; 
} decrip_format;

typedef struct {
	int id;
	std::function<int(std::vector<std::string>)> creator;
} code_gen;

typedef std::map<std::string, decrip_format> objfile_format;

typedef std::map<std::string, std::string> objmacro_format;

typedef std::map<std::string, code_gen> code_gen_table;

extern code_gen_table opcode;

objfile_format objfile = {
	// OBJ Header
	{"header_magic", {5, "str", true}},
	{"header_type", {1, "int", true}},
	{"header_order", {1, "int", true}},
	{"header_version", {1, "int", false}},
	{"header_machine", {1, "int", true}},
	// OBJ I/O Configuration Segment
	{"iocs_update_interval", {4, "int", false}},
	{"iocs_ldi_count", {1, "int", false}},
    {"iocs_ldo_count", {1, "int", false}},
    {"iocs_lai_count", {1, "int", false}},
    {"iocs_lao_count", {1, "int", false}},
    {"iocs_rdi_count", {1, "int", false}},
    {"iocs_rdo_count", {1, "int", false}},
    {"iocs_rai_count", {1, "int", false}},
    {"iocs_rao_count", {1, "int", false}},
    // OBJ Servo Configuration Segment
    {"scs_axis_count", {1, "int", false}},
    {"scs_update_interval", {4, "int", false}},
    // OBJ Axis Configuration Segment
    {"acs_name", {16, "str", false}},
    {"acs_id", {1, "int", false}},
    {"acs_type", {1, "int", true}},
    {"acs_combined", {1, "int", true}},
    {"acs_opmode", {1, "int", true}},
    {"acs_min_pos", {8, "float", false}},
    {"acs_max_pos", {8, "float", false}},
    {"acs_max_vel", {8, "float", false}},
    {"acs_max_acc", {8, "float", false}},
    {"acs_max_dec", {8, "float", false}},
    {"acs_max_jerk", {8, "float", false}},
    // OBJ PLC Task List Segment
    {"plc_task_count", {1, "int", false}},
    {"plc_global_count", {2, "int", false}},
    {"plc_timer_count", {2, "int", false}},
    // OBJ PLC Task Description Segment
    {"tds_name",{16, "str", false}},
    {"tds_priority",{1, "int", false}},
    {"tds_type",{1, "int", true}},
    {"tds_signal",{2, "int", false}},
    {"tds_interval",{4, "int", false}},
    {"tds_sp_size",{4, "int", false}},
    {"tds_cs_size",{2, "int", false}},
    {"tds_pou_count",{2, "int", false}},
    {"tds_const_count",{2, "int", false}},
    {"tds_global_count",{2, "int", false}},
    {"tds_refval_count", {2, "int", false}},
    {"tds_inst_count",{4, "int", false}},
    // OBJ PLC Task User-level POU Description Segment
    {"pds_name",{20, "str", false}},
    {"pds_type", {1, "int", true}},
    {"pds_instance", {4, "int", false}},
    {"pds_input_count",{1, "int", false}},
    {"pds_inout_count",{1, "int", false}},
    {"pds_output_count",{1, "int", false}},
    {"pds_local_count",{1, "int", false}},
    {"pds_entry",{4, "int", false}}
};

objmacro_format objmacro = {
	// OBJ Header
	{"MAGIC", "\x7fPLC\0"},
	{"SYS_TYPE_32", "1"},
	{"SYS_TYPE_64", "2"},
	{"BYTE_ORDER_LIT", "1"},
	{"BYTE_ORDER_BIT", "2"},
	{"MACH_CORTEX_A8", "1"},
	// Axis 
	{"AXIS_TYPE_FINITE", "1"},
	{"AXIS_TYPE_MODULO", "2"},
	{"AXIS_INDEPENDENT", "1"},
	{"AXIS_COMBINED", "2"},
	{"OPMODE_POS", "1"},
	{"OPMODE_VEL", "2"},
	{"OPMODE_TOR", "3"},
	// task
	{"TASK_TYPE_SIGNAL", "1"},
	{"TASK_TYPE_INTERVAL", "2"},
	// pou 
	{"POU_TYPE_FUN", "1"},
	{"POU_TYPE_FB", "2"},
	{"POU_TYPE_PROG", "3"},
	// OBJ PLC Task Constant/Global Value Type
	{"TINT", "1"},
	{"TUINT", "2"},
	{"TDOUBLE", "3"},
	{"TSTRING", "4"},
	// System-level POU
    {"SFUN_ABS", "0"},
    {"SFUN_SQRT", "1"}
};

// Original Instruction Encoder
auto create_ABC = []( std::vector<std::string> result) {
					return opcode[result[1]].id << POS_OP \
							| std::stoi(result[2]) << POS_A \
							| std::stoi(result[3]) << POS_B \
							| std::stoi(result[4]) << POS_C;
					};

auto create_ABx = []( std::vector<std::string> result) {
					return opcode[result[1]].id << POS_OP \
							| std::stoi(result[2]) << POS_A \
							| std::stoi(result[3]) << POS_Bx;
					};

auto create_sAx = []( std::vector<std::string> result) {
					return opcode[result[1]].id << POS_OP \
							| std::stoi(result[2])+BIAS_sAx << POS_sAx;
					};

// Helper Instruction Encoder
auto create_DX = []( std::vector<std::string> result) -> int {
						std::vector<std::string> new_res = result;
						new_res[3] = std::to_string(std::stoi(result[3]) * 8 + std::stoi(result[4]));
						new_res[4] = std::to_string(1);
						return create_ABC(new_res);
					};

auto create_DB = []( std::vector<std::string> result) -> int {
						std::vector<std::string> new_res = result;
						new_res[3] = std::to_string(std::stoi(result[3]) * 8);
						new_res[4] = std::to_string(8);
						return create_ABC(new_res);
					};

auto create_DW = []( std::vector<std::string> result) -> int {
						std::vector<std::string> new_res = result;
						new_res[3] = std::to_string(std::stoi(result[3]) * 16);
						new_res[4] = std::to_string(16);
						return create_ABC(new_res);
					};

auto create_DD = []( std::vector<std::string> result) -> int {
						std::vector<std::string> new_res = result;
						new_res[3] = std::to_string(std::stoi(result[3]) * 32);
						new_res[4] = std::to_string(32);
						return create_ABC(new_res);
					};

auto create_scall = []( std::vector<std::string> result) -> int {
						std::vector<std::string> new_res = result;
						new_res[3] = objmacro[result[3]];
						return create_ABx(new_res);
					};		

code_gen_table opcode = {
	// data move
	{"OP_GLOAD", {1, create_ABx}},
	{"OP_GSTORE", {2, create_ABx}},
	{"OP_KLOAD", {3, create_ABx}},
    {"OP_LDLOAD", {4,  create_ABC}},
    {"OP_LDSTORE", {5, create_ABC}},
    {"OP_LALOAD", {6,  create_ABC}},
    {"OP_LASTORE", {7, create_ABC}},
    {"OP_RDLOAD", {8, create_ABC}},
    {"OP_RDSTORE", {9, create_ABC}},
    {"OP_RALOAD", {10, create_ABC}},
    {"OP_RASTORE", {11, create_ABC}},
    {"OP_MOV", {12, create_ABC}},
    //arithmetic
    {"OP_ADD", {13, create_ABC}},
    {"OP_SUB", {14, create_ABC}},
    {"OP_MUL", {15, create_ABC}},
    {"OP_DIV", {16, create_ABC}},
    // bit operation
    {"OP_SHL", {17, create_ABC}},
    {"OP_SHR", {18, create_ABC}},
    {"OP_AND", {19, create_ABC}},
    {"OP_OR",  {20, create_ABC}},
    {"OP_XOR", {21, create_ABC}},
    {"OP_NOT", {22, create_ABC}},
    // logic operation
    {"OP_LAND", {23, create_ABC}},
    {"OP_LOR",  {24, create_ABC}},
    {"OP_LXOR", {25, create_ABC}},
    {"OP_LNOT", {26, create_ABC}},
    // comparison
    {"OP_LT", {27, create_ABC}},
    {"OP_LE", {28, create_ABC}},
    {"OP_GT", {29, create_ABC}},
    {"OP_GE", {30, create_ABC}},
    {"OP_EQ", {31, create_ABC}},
    {"OP_NE", {32, create_ABC}},
    // flow control
    {"OP_CONDJ", {33, create_ABx}},
    {"OP_JMP", {34, create_sAx}},
    {"OP_HALT", {35, create_ABC}},
    // call
    {"OP_SCALL", {36, create_scall}},
    {"OP_UCALL", {37, create_ABx}},
    {"OP_RET", {38, create_ABx}},
    // reference data manipulation
    {"OP_GETFIELD", {39, create_ABC}},
    {"OP_SETFIELD", {40, create_ABC}},
    // functional instruction
    {"OP_TP", {41, create_ABx}},
    {"OP_TON", {42, create_ABx}},
    {"OP_TOF", {43, create_ABx}},
    // helper
    {"OP_LDIX", {4, create_DX}},
    {"OP_LDIB", {4, create_DB}},
    {"OP_LDIW", {4, create_DW}},
    {"OP_LDID", {4, create_DD}},
    {"OP_LDOX", {5, create_DX}},
    {"OP_LDOB", {5, create_DB}},
    {"OP_LDOW", {5, create_DW}},
    {"OP_LDOD", {5, create_DD}},
    {"OP_RDIX", {8, create_DX}},
    {"OP_RDIB", {8, create_DB}},
    {"OP_RDIW", {8, create_DW}},
    {"OP_RDID", {8, create_DD}},
    {"OP_RDOX", {9, create_DX}},
    {"OP_RDOB", {9, create_DB}},
    {"OP_RDOW", {9, create_DW}},
    {"OP_RDOD", {9, create_DD}}
};

//字符串分割函数
std::vector<std::string> split(std::string str,std::string pattern)
{
  std::string::size_type pos;
  std::vector<std::string> result;
  std::vector<std::string> realresult;
  str += pattern;//扩展字符串以方便操作
  int size=str.size();
 
  for(int i = 0; i < size; i ++)
  {
    pos = str.find(pattern, i);
    if(pos < size)
    {
      std::string s = str.substr(i, pos - i);
      result.push_back(s);
      i = pos+pattern.size() - 1;
    }
  }
  for(auto elem : result) {
  	if(!elem.empty()){
  		realresult.push_back(elem);
  	}
  }
  return realresult;
}


void dump_inst(std::ofstream &outfile, std::vector<std::string> &result) {
	int temp = opcode[result[1]].creator(result);
	outfile.write((char*)&temp, 4);
	std::cout << "instruction" << std::endl;
}

void dump_value(std::ofstream &outfile, std::vector<std::string> &result) {
	if(result[1] == "TUINT") {
		outfile << (char)2;
		unsigned long tmp = std::stoul(result[2]);
		outfile.write((char*)&tmp, 8);
	} else if(result[1] == "TINT") {
		outfile << (char)1;
		long tmp = std::stol(result[2]);
		outfile.write((char*)&tmp, 8);
	} else if(result[1] == "TDOUBLE") {
		outfile << (char)3;
		double tmp = std::stod(result[2]);
		outfile.write((char*)&tmp, 8);
	} else if(result[1] == "TSTRING") {
		outfile << (char)4;
	} else if(result[1] == "S" || result[1] == "A" || result[1] == "FB" ) {
		outfile << (char)5;
		unsigned long tmp = std::stoul(result[2]);
		outfile.write((char*)&tmp, 8);
	}
}

void dump_ref_value(std::ofstream &outfile, std::vector<std::string> &result) {
	std::vector<std::string> new_res{"", "", ""};
	int temp = std::stoi(result[1]);
	unsigned long tmp = std::stoul(result[1]);
	std::cout << tmp << std::endl;
	outfile.write((char*)&tmp, 2);
	if(result[0] == "S") {
		int cnt = 2;
		for(int i = 0; i < temp; i ++) {
			new_res[1] = result[cnt++];
			new_res[2] = result[cnt++];
			dump_value(outfile, new_res);
		}
	} else if(result[0] == "A") {
		int cnt = 3;
		for(int i = 0; i < temp; i ++) {
			new_res[1] = result[2];
			new_res[2] = result[cnt++];
			dump_value(outfile, new_res);
		}
	} else {
		int cnt = 2;
		for(int i = 0; i < temp; i ++) {
			new_res[1] = result[cnt++];
			new_res[2] = result[cnt++];
			dump_value(outfile, new_res);
		}
	}
}

void dump_obj(std::ofstream &outfile, std::vector<std::string> &result) {
	if(result[0] == "K" || result[0] == "G" || result[0] == "PG") {
    	dump_value(outfile, result);
	} else if (result[0] == "S" || result[0] == "A" || result[0] == "FB") {
		dump_ref_value(outfile, result);
    } else if(result[0] == "I") {
    	dump_inst(outfile, result);
    } else {
    	decrip_format field = objfile[result[0]];
    	if(field.is_macro == true) {
    		result[1] = objmacro[result[1]];
    	}
    	if(field.type == "str") {
    		outfile << std::setw(field.bytes) << std::setfill('\0') << std::setiosflags(std::ios::left) << result[1];
    	} else if(field.type == "int") {
    		int temp = std::stoi(result[1]);
			outfile.write((char*)&temp, field.bytes);
    	} else if(field.type == "float") {
    		double temp = std::stod(result[1]);
			outfile.write((char*)&temp, field.bytes);
    	}
    }
}

int main(int argc, const char* argv[]) {
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
	outfile.open("exec.obj", std::ofstream::binary);
	while(!infile.eof()) {
		getline(infile, strline);
		result = split(strline, " ");

#ifdef DEBUG
		std::cout << result.size() << ": ";
		for(auto elem : result) {
			std::cout << elem << " ";
		}
		std::cout << std::endl;
#endif
		if((result.size() > 0) && result[0] == "EOF") {
			break;
		}
		if((result.size() > 0) && (result[0] != "#")) {
			dump_obj(outfile, result);
		}
	}
	infile.close();
	outfile.close();
	return 0;
}