#include <iostream>
#include <ctime>
#include "circuit.h"
#include "GetLongOpt.h"
#include "ReadPattern.h"
using namespace std;

// All defined in readcircuit.l
extern char* yytext;
extern FILE *yyin;
extern CIRCUIT Circuit;
extern int yyparse (void);
extern bool ParseError;

extern void Interactive();

GetLongOpt option;

int SetupOption(int argc, char ** argv)
{
    // original functions
    option.usage("[options] input_circuit_file");
    option.enroll("help", GetLongOpt::NoValue,
            "print this help summary", 0);
    option.enroll("logicsim", GetLongOpt::NoValue,
            "run logic simulation", 0);
    option.enroll("plogicsim", GetLongOpt::NoValue,
            "run parallel logic simulation", 0);
    option.enroll("fsim", GetLongOpt::NoValue,
            "run stuck-at fault simulation", 0);
    option.enroll("stfsim", GetLongOpt::NoValue,
            "run single pattern single transition-fault simulation", 0);
    option.enroll("transition", GetLongOpt::NoValue,
            "run transition-fault ATPG", 0);
    option.enroll("input", GetLongOpt::MandatoryValue,
            "set the input pattern file", 0);
    option.enroll("output", GetLongOpt::MandatoryValue,
            "set the output pattern file", 0);
    option.enroll("bt", GetLongOpt::NoValue,
            "set the backtrack limit", 0);
    // ass0
    option.enroll("ass0", GetLongOpt::NoValue, 
            "Run assignment 0 for the specified circuit", 0);
    // ass1
    option.enroll("path", GetLongOpt::NoValue,
            "List the possible paths from start to end", 0);
    option.enroll("start",GetLongOpt::MandatoryValue,
            "get the starting gate for the path", 0);
    option.enroll("end",GetLongOpt::MandatoryValue,
            "get the ending gate for the path", 0);    
    // ass2
    option.enroll("pattern", GetLongOpt::NoValue,
            			"Generate random pattern", 0);
    option.enroll("num",GetLongOpt::MandatoryValue,
                                "specify the number of the generated pattern", 0);
    option.enroll("unknown", GetLongOpt::NoValue,
            			"Generate random pattern with unknown", 0);
    option.enroll("mod_logicsim", GetLongOpt::NoValue,
            			"use cpu instructions to compute AND, OR and NOT", 0);
    // ass3
    option.enroll("packed_sim", GetLongOpt::NoValue,
            			"pack (1, 4, 8, 16) patterns into a simulation run", 0);                            
    option.enroll("simulator", GetLongOpt::MandatoryValue,
            "Generate a compiled code simulator", 0);         
    // ass4
    option.enroll("check_point", GetLongOpt::NoValue,
            			"Generate Fault list with checkpoint theorem", 0);             
    option.enroll("bridging", GetLongOpt::NoValue,
            			"Generate Fault list with bridging fault", 0);                                              
                   
    // IO 
    int optind = option.parse(argc, argv);
    if ( optind < 1 ) { exit(0); }
    if ( option.retrieve("help") ) {
        option.usage();
        exit(0);
    }
    return optind;
}

int main(int argc, char ** argv)
{
    int optind = SetupOption(argc, argv);
    clock_t time_init, time_end;
    time_init = clock();
    //Setup File
    if (optind < argc) {
        if ((yyin = fopen(argv[optind], "r")) == NULL) {
            cout << "Can't open circuit file: " << argv[optind] << endl;
            exit( -1);
        }
        else {
            string circuit_name = argv[optind];
            string::size_type idx = circuit_name.rfind('/');
            if (idx != string::npos) { circuit_name = circuit_name.substr(idx+1); }
            idx = circuit_name.find(".bench");
            if (idx != string::npos) { circuit_name = circuit_name.substr(0,idx); }
            Circuit.SetName(circuit_name);
        }
    }
    else {
        cout << "Input circuit file missing" << endl;
        option.usage();
        return -1;
    }
    cout << "Start parsing input file\n";
    yyparse();
    if (ParseError) {
        cerr << "Please correct error and try Again.\n";
        return -1;
    }
    fclose(yyin);
    Circuit.FanoutList();
    Circuit.SetupIO_ID();
    Circuit.Levelize();
    Circuit.Check_Levelization();
    Circuit.InitializeQueue();

    if (option.retrieve("logicsim")) {
        //logic simulator
        Circuit.InitPattern(option.retrieve("input"));
        Circuit.LogicSimVectors();
    }
    else if (option.retrieve("plogicsim")) {
        //parallel logic simulator
        Circuit.InitPattern(option.retrieve("input"));
        Circuit.ParallelLogicSimVectors();
    }
    else if (option.retrieve("stfsim")) {
        //single pattern single transition-fault simulation
        Circuit.MarkOutputGate();
        Circuit.GenerateAllTFaultList();
        Circuit.InitPattern(option.retrieve("input"));
        Circuit.TFaultSimVectors();
    }
    else if (option.retrieve("transition")) {
        Circuit.MarkOutputGate();
        Circuit.GenerateAllTFaultList();
        Circuit.SortFaninByLevel();
        if (option.retrieve("bt")) {
            Circuit.SetBackTrackLimit(atoi(option.retrieve("bt")));
        }
        Circuit.TFAtpg();
    }
    // Ass0: print circuit's infomation
    else if (option.retrieve("ass0")) {
        Circuit.printINFO();
    }
    // Ass1: find all path from start to end in circuit
    else if (option.retrieve("path")){
        // GetLongOpt::MandatoryValue would be const char * as defined
        const char *startGatename = option.retrieve("start");
        const char *endGatename = option.retrieve("end");
        Circuit.findpath(startGatename, endGatename);
    }
    // ASS2: generate random patterns and Logicsim
    else if (option.retrieve("pattern")){
        string circuitname = Circuit.GetName();
        string outputfilename = "empty";
        int num = 0;

        // set parameters
        if (option.retrieve("num")&&option.retrieve("output"))
        {
            num = atoi(option.retrieve("num"));
            outputfilename = "./input/" + static_cast<string>(option.retrieve("output"));
        }
        else{
            cerr << "Please Ensure you have -num [number of pattern] for your command" <<endl;
            cerr << "Please Ensure you have -output [output file name] for your command" <<endl;
            return 0;
        }

        // generating patterns
        if(option.retrieve("unknown")){
            cout << "Generating "<<circuitname<<"'s input pattern for "<<num<<" patterns (with unknown)"<<endl;
            Circuit.GenerateRandomPatternWithUnknown(num, circuitname);
        }
        else{
            cout << "Generating "<<circuitname<<"'s input pattern for "<<num<<" patterns (without unknown)"<<endl;
            Circuit.GenerateRandomPattern(num, circuitname);
        }

        
        // Read generated file
        Circuit.InitPattern(outputfilename.c_str());
        Circuit.LogicSimVectors();
    }
    // Ass2: modified logicsim
    else if (option.retrieve("mod_logicsim")){
        Circuit.InitPattern(option.retrieve("input"));
        Circuit.ModLogicSimVectors();
    }
    // Ass3: Pack (1, 4, 8, 16) patterns in a simulation run
    else if (option.retrieve("packed_sim")){
        Circuit.PackedSim();
        return 0;
    }
    // Ass3: Generate Compiled code simulator
    else if(option.retrieve("simulator")){
        string file_name = (string) option.retrieve("simulator");
        string output_name;

        string::size_type idx;
        idx = file_name.find(".cc");
        if (idx != string::npos) { output_name = file_name.substr(0,idx); }
        output_name.append(".out");
        Circuit.setOutputName(output_name);

        Circuit.InitPattern(option.retrieve("input"));
		Circuit.openSimFile(file_name);
        Circuit.genCompiledCodeSimulator();
        
    }
    // Ass4: Generate fault list with checkpoint throrem
    else if (option.retrieve("check_point")) {
        //generate all stuck-at fault list
        Circuit.CheckpointFaultList();
        Circuit.SortFaninByLevel();
        Circuit.MarkOutputGate();
        //stuck-at fualt ATPG
        Circuit.Atpg();

    }  
    // Ass4: Generate fault list with checkpoint throrem
    else if (option.retrieve("bridging")){
        Circuit.BridgingFaultList();
        return 0;
    }
    else {
        Circuit.GenerateAllFaultList();
        Circuit.SortFaninByLevel();
        Circuit.MarkOutputGate();
        if (option.retrieve("fsim")) {
            //stuck-at fault simulator
            Circuit.InitPattern(option.retrieve("input"));
            Circuit.FaultSimVectors();
        }

        else {
            if (option.retrieve("bt")) {
                Circuit.SetBackTrackLimit(atoi(option.retrieve("bt")));
            }
            //stuck-at fualt ATPG
            Circuit.Atpg();
        }
    }
    time_end = clock();
    cout << "total CPU time = " << double(time_end - time_init)/CLOCKS_PER_SEC << endl;
    cout << endl;
    return 0;
}
