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
    option.enroll("ass0", GetLongOpt::MandatoryValue, 
    "run assignment 0 for the specified circuit", 0);

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
    else if (option.retrieve("ass0")) {
        //PI=0, PO=1, PPI=2, PPO=3, NOT=4, AND=5, NAND=6, OR=7, NOR=8
        int NOT=0, AND=0, NAND=0, BUF=0, NOR=0, OR = 0,DFF=0;
        for(int i = 0; i < Circuit.No_Gate();i++){
            // calculate num of diff types of gates
            // in s713.bench, there are 35 inputs and 19 dff and 23 outputs
            // but i got 35 + 19 = 54 BUF and 23 + 19 = 42 outputs 
            // which means DFF will be shown as 0 and 1 at the same time
            if(Circuit.Gate(i)->GetFunction() == 0 || Circuit.Gate(i)->GetFunction() == 1){
                BUF = BUF + 1;
            }
            else if (Circuit.Gate(i)->GetFunction() == 4){
                NOT = NOT + 1;
            }
            else if (Circuit.Gate(i)->GetFunction() == 5){
                AND = AND + 1;
            }
            else if (Circuit.Gate(i)->GetFunction() == 6){
                NAND = NAND + 1;
            }
            else if (Circuit.Gate(i)->GetFunction() == 7){
                OR = OR + 1;
            }
            else if (Circuit.Gate(i)->GetFunction() == 8){
                NOR = NOR + 1;
            }
        }

        cout <<  "Gate name: " << Circuit.GetName() << endl;
        cout <<  "Number of inputs: " << Circuit.No_PI() << endl;
        cout <<  "Number of outputs: " << Circuit.No_PO() << endl;
        cout <<  "Total number of gates: " << NOR+OR+AND+NAND+NOT << endl;
        cout <<  "Number of gates for each type" <<endl;
        cout << "NOT: " << NOT <<"      " << "AND: " << AND <<"      " << "NAND: "<< NAND <<endl;
        cout << "NOR: " << NOR <<"      " << "OR: "  << OR <<endl;
        // I found that in seq model, the num of PPI or PPO will be the same as DFF, so I use it.
        // But I'm not able to fine the exact position of DFF, I wonder why.
        cout << "DFF: " << Circuit.No_PPI() << endl;
        
        int num_nets = 0, num_bran = 0, num_stem = 0;
        int temp=0, tempbr=0;
        float out = 0, avgout = 0;
        for(int i = 0; i < Circuit.No_Gate();i++){
            cout << Circuit.Gate(i)-> GetName() << "'s func: " << Circuit.Gate(i)->GetFunction()<<endl;
            if (Circuit.Gate(i)->No_Fanout() >= 2){
                temp = Circuit.Gate(i)->No_Fanout() +1;
                tempbr = Circuit.Gate(i)->No_Fanout();
                num_stem += 1;
            }
            else{
                temp = Circuit.Gate(i)->No_Fanout();
                tempbr = 0;
            }
            num_bran += tempbr;
            num_nets = num_nets + temp;
            out += Circuit.Gate(i)->No_Fanout();
        }

        avgout = out / Circuit.No_Gate();
        cout <<  "Total number of signal nets: " <<  num_nets << endl;
        cout <<  "Number of branch nets: " << num_bran <<endl;
        cout <<  "Number of stem nets: " << num_stem <<endl;
        cout <<  "Average number of fanouts of each gate: "  << avgout <<endl ;

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
