/* Logic Simulator
 * Last update: 2006/09/20 */
#include <iostream>
#include <fstream>
#include <filesystem> // C++17 required
#include <ctime>
#include <cstdlib> 
#include <random>
#include <sys/types.h>
#include <unistd.h>
#include "gate.h"
#include "circuit.h"
#include "ReadPattern.h"
#include "GetLongOpt.h"
using namespace std;

extern GetLongOpt option;

// Do logic simulation for test patterns
// e.g. PI G1 PI G2 PI G3 PI G4 PI G5 
//      01X11
void CIRCUIT::LogicSimVectors()
{
    cout << "Run logic simulation" << endl;
    //read test patterns
    while (!Pattern.eof()) {
        Pattern.ReadNextPattern();
        PatternCount ++;
        SchedulePI();
        LogicSim();
        PrintIO();
    }
    Pattern.ResetPattern();
    Pattern.ClosePatternFile();
    return;
}

//do event-driven logic simulation
void CIRCUIT::LogicSim()
{
    GATE* gptr;
    VALUE new_value;
    // level 0 = PI, Maxlevel = PO (?)
    for (unsigned i = 0;i <= MaxLevel;i++) {
        while (!Queue[i].empty()) {
            gptr = Queue[i].front();
            Queue[i].pop_front();
            gptr->ResetFlag(SCHEDULED);
            new_value = Evaluate(gptr);
            if (new_value != gptr->GetValue()) {
                gptr->SetValue(new_value);
                ScheduleFanout(gptr);
            }
        }
    }
    return;
}

//Used only in the first pattern
void CIRCUIT::SchedulePI()
{
    for (unsigned i = 0;i < No_PI();i++) {
        if (PIGate(i)->GetFlag(SCHEDULED)) {
            PIGate(i)->ResetFlag(SCHEDULED);
            ScheduleFanout(PIGate(i));
        }
    }
    return;
}

//schedule all fanouts of PPIs to Queue
void CIRCUIT::SchedulePPI()
{
    for (unsigned i = 0;i < No_PPI();i++) {
        if (PPIGate(i)->GetFlag(SCHEDULED)) {
            PPIGate(i)->ResetFlag(SCHEDULED);
            ScheduleFanout(PPIGate(i));
        }
    }
    return;
}

//set all PPI as 0
void CIRCUIT::SetPPIZero()
{
    GATE* gptr;
    for (unsigned i = 0;i < No_PPI();i++) {
        gptr = PPIGate(i);
        if (gptr->GetValue() != S0) {
            gptr->SetFlag(SCHEDULED);
            gptr->SetValue(S0);
        }
    }
    return;
}

//schedule all fanouts of gate to Queue
void CIRCUIT::ScheduleFanout(GATE* gptr)
{
    for (unsigned j = 0;j < gptr->No_Fanout();j++) {
        Schedule(gptr->Fanout(j));
    }
    return;
}

//initial Queue for logic simulation
void CIRCUIT::InitializeQueue()
{
    SetMaxLevel();
    Queue = new ListofGate[MaxLevel + 1];
    return;
}

//evaluate the output value of gate
VALUE CIRCUIT::Evaluate(GATEPTR gptr)
{
    GateEvalCount++;  // 每次評估時增加計數
    GATEFUNC fun(gptr->GetFunction());  // G_AND, G_NAND, G_OR, G_NOR
    VALUE cv(CV[fun]); //controlling value
    VALUE value(gptr->Fanin(0)->GetValue());
    switch (fun) {
        case G_AND:
        case G_NAND:
            for (unsigned i = 1;i<gptr->No_Fanin() && value != cv;++i) {
                value = AndTable[value][gptr->Fanin(i)->GetValue()];
            }
            break;
        case G_OR:
        case G_NOR:
            for (unsigned i = 1;i<gptr->No_Fanin() && value != cv;++i) {
                value = OrTable[value][gptr->Fanin(i)->GetValue()];
            }
            break;
        default: break;
    }
    //NAND, NOR and NOT
    if (gptr->Is_Inversion()) { value = NotTable[value]; }
    return value;
}

extern GATE* NameToGate(string);

void PATTERN::close()
{
    patterninput.close();
}

void PATTERN::Initialize(char* InFileName, int no_pi, string TAG)
{
    patterninput.open(InFileName, ios::in);
    if (!patterninput) {
        cout << "Unable to open input pattern file: " << InFileName << endl;
        exit( -1);
    }
    string piname;
    while (no_pi_infile < no_pi) {
        patterninput >> piname;
        if (piname == TAG) {
            patterninput >> piname;
            inlist.push_back(NameToGate(piname));
            no_pi_infile++;
        }
        else {
            cout << "Error in input pattern file at line "
                << no_pi_infile << endl;
            cout << "Maybe insufficient number of input\n";
            exit( -1);
        }
    }
    
    return;
}

//Assign next input pattern to PI
void PATTERN::ReadNextPattern()
{
    char V;
    for (int i = 0;i < no_pi_infile;i++) {
        patterninput >> V;
        if (V == '0') {
            if (inlist[i]->GetValue() != S0) {
                inlist[i]->SetFlag(SCHEDULED);
                inlist[i]->SetValue(S0);
            }
        }
        else if (V == '1') {
            if (inlist[i]->GetValue() != S1) {
                inlist[i]->SetFlag(SCHEDULED);
                inlist[i]->SetValue(S1);
            }
        }
        else if (V == 'X') {
            if (inlist[i]->GetValue() != X) {
                inlist[i]->SetFlag(SCHEDULED);
                inlist[i]->SetValue(X);
            }
        }
    }
    //Take care of newline to force eof() function correctly
    patterninput >> V;
    if (!patterninput.eof()) patterninput.unget();
    return;
}

void CIRCUIT::PrintIO()
{
    register unsigned i;
    for (i = 0;i<No_PI();++i) { cout << PIGate(i)->GetValue(); }
    cout << " ";
    for (i = 0;i<No_PO();++i) { cout << POGate(i)->GetValue(); }
    cout << endl;
    return;
}

void CIRCUIT::GenerateRandomPattern(int num, string circuitname){
    unsigned i,j;
    random_device rd;  
    mt19937 gen(rd()); 
    uniform_int_distribution<> dis(0, 1); 

    filesystem::create_directories("./input/");

    string filepath = "./input/" + circuitname + ".input";

    ofstream outfile(filepath);

    if (!outfile.is_open()) {
        cerr << "Failed to open file: " << filepath << endl;
        return;
    }

   
    for (i = 0; i < No_PI(); ++i) {
        outfile << "PI " << Gate(i)->GetName() << " ";
    }
    outfile << endl;
    for(i = 0; i < num; i++){
        for(j = 0; j < No_PI(); j++){
            int random = dis(gen); 
            outfile << random;
        }
        outfile << endl;
    }
    outfile.close();
    cout << "Pattern saved to " << filepath << endl;
}

void CIRCUIT::GenerateRandomPatternWithUnknown(int num, string circuitname){
    unsigned i,j;
    random_device rd;  
    mt19937 gen(rd()); 
    uniform_int_distribution<> dis(0, 2); 

    
    filesystem::create_directories("./input/");

    string filepath = "./input/" + circuitname + ".input";
    
    ofstream outfile(filepath);

    if (!outfile.is_open()) {
        cerr << "Failed to open file: " << filepath << endl;
        return;
    }

    // write file
    for (i = 0; i < No_PI(); ++i) {
        outfile << "PI " << Gate(i)->GetName() << " ";
    }
    outfile << endl;
    for(i = 0; i < num; i++){
        for(j = 0; j < No_PI(); j++){
            int random = dis(gen);
            if(random == 2)
                outfile <<'X';
            else
                outfile << random;
        }
        outfile << endl;
    }
    
    outfile.close();
    cout << "Pattern saved to " << filepath << endl;
}

void CIRCUIT::ModLogicSimVectors(){
    cout << "Run Modified logic simulation" << endl;
    //read test patterns
    while (!Pattern.eof()) {
        Pattern.ReadNextPattern();
        SchedulePI();
        ModLogicSim();
        PrintIO();
    }
    Pattern.ResetPattern();
    Pattern.ClosePatternFile();
    return;
}

//do event-driven logic simulation
void CIRCUIT::ModLogicSim()
{
    GATE* gptr;
    VALUE new_value;
    for (unsigned i = 0;i <= MaxLevel;i++) {
        while (!Queue[i].empty()) {
            gptr = Queue[i].front();
            Queue[i].pop_front();
            gptr->ResetFlag(SCHEDULED);
            new_value = ModEvaluate(gptr);
            if (new_value != gptr->GetValue()) {
                gptr->SetValue(new_value);
                ScheduleFanout(gptr);
            }
        }
    }
    return;
}

VALUE CIRCUIT::ModEvaluate(GATEPTR gptr)
{
    GATEFUNC fun(gptr->GetFunction());  // G_AND, G_NAND, G_OR, G_NOR
    VALUE cv(CV[fun]); 
    VALUE value = gptr->Fanin(0)->GetValue();

    switch (fun) {
        case G_AND:
        case G_NAND:
            for (unsigned i = 1; i < gptr->No_Fanin() && value != cv; ++i) {
                VALUE fanin_value = gptr->Fanin(i)->GetValue();

                if (value == X || fanin_value == X) {
                    value = X;
                    break;
                }

                value = static_cast<VALUE>(value & fanin_value);
            }
            break;

        case G_OR:
        case G_NOR:
            for (unsigned i = 1; i < gptr->No_Fanin() && value != cv; ++i) {
                VALUE fanin_value = gptr->Fanin(i)->GetValue();

                if (value == X || fanin_value == X) {
                    value = X;
                    break;
                }

                value = static_cast<VALUE>(value | fanin_value);
            }
            break;

        default:
            break;
    }

    if (gptr->Is_Inversion()) {
        value = static_cast<VALUE>(~value & 0x01);  
    }

    return value;
}   

void CIRCUIT::PackedSim(){
    string name = GetName();
    string outputfilename = "empty";
    
    int pid=(int) getpid();
    char buf[1024];
    int nums[] = {1, 4, 8, 16};
    
    for (int num : nums) {
        clock_t time_init, time_end;
        time_init = clock();
        outputfilename = "./input/" + name + ".input";
    
        cout << "Generating " << name << "'s input pattern for " << num << " patterns (with unknown)" << endl;
        GenerateRandomPatternWithUnknown(num, name);
        InitPattern(outputfilename.c_str());
        LogicSimVectors();
        printEvalResult();
        time_end = clock();
        cout << "total CPU time = " << double(time_end - time_init)/CLOCKS_PER_SEC << endl;
        cout << "Memory: " << std::flush;
        sprintf(buf, "cat /proc/%d/statm", pid);
        system(buf);
        cout << endl;
        PatternCount =0 ;
        cout<<"********************************************************"<<endl<<endl;
    }
}

