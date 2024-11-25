#ifndef CIRCUIT_H
#define CIRCUIT_H
#include "fault.h"
#include "tfault.h"
#include "ReadPattern.h"
#include <stdlib.h>
#include <cstring>
#include <filesystem>
#include <unordered_set>

#define xstr(s) STR(s)
#define STR(s) #s


typedef GATE* GATEPTR;
using namespace std;
namespace fs = std::filesystem;

class CIRCUIT
{
    private:
        string Name;
        PATTERN Pattern;
        vector<GATE*> Netlist;
        vector<GATE*> PIlist; //store the gate indexes of PI
        vector<GATE*> POlist;
        vector<GATE*> PPIlist;
        vector<GATE*> PPOlist;
        list<FAULT*> Flist; //collapsing fault list
        list<FAULT*> UFlist; //undetected fault list
        list<TFAULT*> TFlist; //collapsing fault list
        list<TFAULT*> UTFlist; //undetected fault list
        list<FAULT*> BFlist; // bridging fault list
        unsigned MaxLevel;
        unsigned BackTrackLimit; //backtrack limit for Podem
        typedef list<GATE*> ListofGate;
        typedef list<GATE*>::iterator ListofGateIte;
        ListofGate* Queue;
        ListofGate GateStack;
        ListofGate PropagateTree;
        ListofGateIte QueueIte;
        // Calculate Gate Eval times
        unsigned int GateEvalCount;
        unsigned int PatternCount;
        // Ass3 simulator
		string input_name, output_name;
        ofstream ofsHeader, ofsMain, ofsEva, ofsPrintIO;
        ofstream ofs; 

        

    public:
        //Initialize netlist
        CIRCUIT(): MaxLevel(0), BackTrackLimit(10000) {
            Netlist.reserve(32768);
            PIlist.reserve(128);
            POlist.reserve(512);
            PPIlist.reserve(2048);
            PPOlist.reserve(2048);
            GateEvalCount = 0;
            PatternCount = 0;
        }
        CIRCUIT(unsigned NO_GATE, unsigned NO_PI = 128, unsigned NO_PO = 512,
                unsigned NO_PPI = 2048, unsigned NO_PPO = 2048) {
            Netlist.reserve(NO_GATE);
            PIlist.reserve(NO_PI);
            POlist.reserve(NO_PO);
            PPIlist.reserve(NO_PPI);
            PPOlist.reserve(NO_PPO);
            GateEvalCount = 0;
            PatternCount = 0;
        }
        ~CIRCUIT() {
            for (unsigned i = 0;i<Netlist.size();++i) { delete Netlist[i]; }
            list<FAULT*>::iterator fite;
            for (fite = Flist.begin();fite!=Flist.end();++fite) { delete *fite; }
            if(ofs.is_open())
				ofs.close();
            if(ofsHeader.is_open())
				ofsHeader.close();
			if(ofsMain.is_open())
				ofsMain.close();
			if(ofsEva.is_open())
				ofsEva.close();
			if(ofsPrintIO.is_open())
				ofsPrintIO.close();
        }

        void AddGate(GATE* gptr) { Netlist.push_back(gptr); }
        void SetName(string n){ Name = n;}
        string GetName(){ return Name;}
        int GetMaxLevel(){ return MaxLevel;}
        void SetBackTrackLimit(unsigned bt) { BackTrackLimit = bt; }

        //Access the gates by indexes
        GATE* Gate(unsigned index) { return Netlist[index]; }
        GATE* PIGate(unsigned index) { return PIlist[index]; }
        GATE* POGate(unsigned index) { return POlist[index]; }
        GATE* PPIGate(unsigned index) { return PPIlist[index]; }
        GATE* PPOGate(unsigned index) { return PPOlist[index]; }
        unsigned No_Gate() { return Netlist.size(); }
        unsigned No_PI() { return PIlist.size(); }
        unsigned No_PO() { return POlist.size(); }
        unsigned No_PPI() { return PPIlist.size(); }
        unsigned No_PPO() { return PPOlist.size(); }
        GATEPTR GetGateByName(const std::string& name)
        {
            for (int i = 0; i < No_Gate(); i++) {
                if (Gate(i)->GetName() == name) {
                    return Gate(i);
                }
            }
            return nullptr; // 未找到对应的门
        }

        void InitPattern(const char *pattern) {
            Pattern.Initialize(const_cast<char *>(pattern), PIlist.size(), "PI");
        }

        void Schedule(GATE* gptr)
        {
            if (!gptr->GetFlag(SCHEDULED)) {
                gptr->SetFlag(SCHEDULED);
                Queue[gptr->GetLevel()].push_back(gptr);
            }
        }

        void openSimFile(string file_name) {
            const char* simdir = "sim";

            // filesystem 
            fs::create_directory(simdir);  

            string file_path = string("./") + simdir + "/" + file_name;

            ofs.open(file_path, ofstream::out | ofstream::trunc);
            ofsHeader.open("./sim/header", ofstream::out | ofstream::trunc);
            ofsMain.open("./sim/main", ofstream::out | ofstream::trunc);
            ofsEva.open("./sim/evaluate", ofstream::out | ofstream::trunc);
            ofsPrintIO.open("./sim/printIO", ofstream::out | ofstream::trunc);

            // Check file
            if (!ofs.is_open()) cout << "Cannot open output file!\n";
            if (!ofsHeader.is_open()) cout << "Cannot open header!\n";
            if (!ofsMain.is_open()) cout << "Cannot open main!\n";
            if (!ofsEva.is_open()) cout << "Cannot open evaluate!\n";
            if (!ofsPrintIO.is_open()) cout << "Cannot open printIO!\n";
        }

        //defined in circuit.cc
        void Levelize();
        void FanoutList();
        void Check_Levelization();
        void SetMaxLevel();
        void SetupIO_ID();
            // defined for lab
        void printINFO();
        void findpath(const char* startGatename, const char* endGatename);
        bool dfs(GATE* current, GATE* destination, vector<GATE*>& path, int& path_count);
        void printPath(vector<GATE*>& path);
        void printEvalResult();
        
        
        
        
        //defined in sim.cc
        void SetPPIZero(); //Initialize PPI state
        void InitializeQueue();
        void ScheduleFanout(GATE*);
        void SchedulePI();
        void SchedulePPI();
        void LogicSimVectors();
        void LogicSim();
        void PrintIO();
        VALUE Evaluate(GATEPTR gptr);
            // defined for lab
        void GenerateRandomPatternWithUnknown(int num, string circuitname);
        void GenerateRandomPattern(int num, string circuitname);
        void ModLogicSimVectors();
        void ModLogicSim();
        VALUE ModEvaluate(GATEPTR gptr);
        void PackedSim();
        

        //defined in atpg.cc
        void GenerateAllFaultList();
        void GenerateCheckPointFaultList();
        void GenerateFaultList();
        void Atpg();
        void SortFaninByLevel();
        bool CheckTest();
        bool TraceUnknownPath(GATEPTR gptr);
        bool FaultEvaluate(FAULT* fptr);
        ATPG_STATUS Podem(FAULT* fptr, unsigned &total_backtrack_num);
        ATPG_STATUS SetUniqueImpliedValue(FAULT* fptr);
        ATPG_STATUS BackwardImply(GATEPTR gptr, VALUE value);
        GATEPTR FindPropagateGate();
        GATEPTR FindHardestControl(GATEPTR gptr);
        GATEPTR FindEasiestControl(GATEPTR gptr);
        GATEPTR FindPIAssignment(GATEPTR gptr, VALUE value);
        GATEPTR TestPossible(FAULT* fptr);
        void TraceDetectedStemFault(GATEPTR gptr, VALUE val);
        // defined for lab
        void BridgingFaultList();
        void CheckpointFaultList(); 
        void ReadBridgingFaultList();
        void GenerateSpeFaultList();
        void AtpgWithTrace();
        ATPG_STATUS PodemWithTrace(FAULT* fptr, unsigned &total_backtrack_num);
        void Ass6d();

        //defined in fsim.cc
        void MarkOutputGate();
        void MarkPropagateTree(GATEPTR gptr);
        void FaultSimVectors();
        void FaultSim();
        void FaultSimEvaluate(GATE* gptr);
        bool CheckFaultyGate(FAULT* fptr);
        void InjectFaultValue(GATEPTR gptr, unsigned idx,VALUE value);
        // defined for lab
        void BridgingFaultSimVectors();

	//defined in psim.cc for parallel logic simulation
	void ParallelLogicSimVectors();
	void ParallelLogicSim();
	void ParallelEvaluate(GATEPTR gptr);
	void PrintParallelIOs(unsigned idx);
	void ScheduleAllPIs();
        // defined for lab
    void GenerateCompileCode(string circitname);
    unsigned int getEvaluationCount() {return GateEvalCount;}
    // VLSI-Testing Lab3
    void printStatResult();
    void genCompiledCodeSimulator();
    void ccsParallelLogicSim(bool flag);
    void ccsParallelEvaluate(GATEPTR gptr, bool flag);
    void ccsPrintParallelIOs(unsigned idx);
    void genHeader();
    void genMainBegin();
    void genMainEnd();
    void genEvaBegin();
    void genEvaEnd();
    void genPrintIOBegin();
    void genPrintIOEnd();
    void genIniPattern();
    void combineFilesToOutput();
    void setOutputName(string str) { output_name = str;}

    

	//defined in stfsim.cc for single pattern single transition-fault simulation
	void GenerateAllTFaultList();
	void TFaultSimVectors();
	void TFaultSim_t();
	void TFaultSim();
	bool CheckTFaultyGate(TFAULT* fptr);
	bool CheckTFaultyGate_t(TFAULT* fptr);
	VALUE Evaluate_t(GATEPTR gptr);
	void LogicSim_t();
        void PrintTransition();
        void PrintTransition_t();
        void PrintIO_t();
            // defined for lab

	//defined in tfatpg.cc for transition fault ATPG
	void TFAtpg();
	ATPG_STATUS Initialization(GATEPTR gptr, VALUE target, unsigned &total_backtrack_num);
	ATPG_STATUS BackwardImply_t(GATEPTR gptr, VALUE value);
	GATEPTR FindPIAssignment_t(GATEPTR gptr, VALUE value);
	GATEPTR FindEasiestControl_t(GATEPTR gptr);
	GATEPTR FindHardestControl_t(GATEPTR gptr);
        // defined for lab
};
#endif
