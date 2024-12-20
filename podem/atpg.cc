/* stuck-at fault ATPG for combinational circuit
 * Last update: 2006/12/09 */
#include <iostream>
#include "circuit.h"
#include "GetLongOpt.h"
#include <algorithm>   
#include <regex>

using namespace std;

extern GetLongOpt option;

//generate all stuck-at fault list
void CIRCUIT::GenerateAllFaultList()
{
    cout << "Generate stuck-at fault list" << endl;
    register unsigned i, j;
    GATEFUNC fun;
    GATEPTR gptr, fanout;
    FAULT *fptr;
    for (i = 0;i<No_Gate();++i) {
        gptr = Netlist[i]; 
        fun = gptr->GetFunction();
        if (fun == G_PO) { continue; } //skip PO
        //add stem stuck-at 0 fault to Flist
        fptr = new FAULT(gptr, gptr, S0);
        Flist.push_front(fptr);
        //add stem stuck-at 1 fault to Flist
        fptr = new FAULT(gptr, gptr, S1);
        Flist.push_front(fptr);

        if (gptr->No_Fanout() == 1) { continue; } //no branch faults

        //add branch fault
        for (j = 0;j< gptr->No_Fanout();++j) {
            fanout = gptr->Fanout(j);
            fptr = new FAULT(gptr, fanout, S0);
            fptr->SetBranch(true);
            Flist.push_front(fptr);
            fptr = new FAULT(gptr, fanout, S1);
            fptr->SetBranch(true);
            Flist.push_front(fptr);
        } //end all fanouts
    } //end all gates
    //copy Flist to undetected Flist (for fault simulation)
    UFlist = Flist;
    return;
}

//stuck-at fualt PODEM ATPG (fault dropping)
void CIRCUIT::Atpg()
{
    cout << "Run stuck-at fault ATPG" << endl;
    unsigned i, total_backtrack_num(0), pattern_num(0);
    ATPG_STATUS status;
    FAULT* fptr;
    list<FAULT*>::iterator fite;
    
    //Prepare the output files
    ofstream OutputStrm;
    if (option.retrieve("output")){
        OutputStrm.open((char*)option.retrieve("output"),ios::out);
        if(!OutputStrm){
              cout << "Unable to open output file: "
                   << option.retrieve("output") << endl;
              cout << "Unsaved output!\n";
              exit(-1);
        }
    }

    if (option.retrieve("output")){
	    for (i = 0;i<PIlist.size();++i) {
		OutputStrm << "PI " << PIlist[i]->GetName() << " ";
	    }
	    OutputStrm << endl;
    }
    cout<<"backtracklimit = "<<BackTrackLimit <<endl;
    for (fite = Flist.begin(); fite != Flist.end();++fite) {
        fptr = *fite;
        if (fptr->GetStatus() == DETECTED) { continue; }
        //run podem algorithm
        status = Podem(fptr, BackTrackLimit);
        switch (status) {
            case TRUE:
                fptr->SetStatus(DETECTED);
                ++pattern_num;
                //run fault simulation for fault dropping
                for (i = 0;i < PIlist.size();++i) { 
			ScheduleFanout(PIlist[i]); 
                        if (option.retrieve("output")){ OutputStrm << PIlist[i]->GetValue();}
		}
                if (option.retrieve("output")){ OutputStrm << endl;}
                for (i = PIlist.size();i<Netlist.size();++i) { Netlist[i]->SetValue(X); }
                LogicSim();
                FaultSim();
                break;
            case CONFLICT:
                fptr->SetStatus(REDUNDANT);
                break;
            case FALSE:
                fptr->SetStatus(ABORT);
                break;
        }
    } //end all faults

    //compute fault coverage
    unsigned total_num(0);
    unsigned abort_num(0), redundant_num(0), detected_num(0);
    unsigned eqv_abort_num(0), eqv_redundant_num(0), eqv_detected_num(0);
    for (fite = Flist.begin();fite!=Flist.end();++fite) {
        fptr = *fite;
        switch (fptr->GetStatus()) {
            case DETECTED:
                ++eqv_detected_num;
                detected_num += fptr->GetEqvFaultNum();
                break;
            case REDUNDANT:
                ++eqv_redundant_num;
                redundant_num += fptr->GetEqvFaultNum();
                break;
            case ABORT:
                ++eqv_abort_num;
                abort_num += fptr->GetEqvFaultNum();
                break;
            default:
                cerr << "Unknown fault type exists" << endl;
                break;
        }
    }
    total_num = detected_num + abort_num + redundant_num;

    cout.setf(ios::fixed);
    cout.precision(2);
    cout << "---------------------------------------" << endl;
    cout << "Test pattern number = " << pattern_num << endl;
    cout << "Total backtrack number = " << total_backtrack_num << endl;
    cout << "---------------------------------------" << endl;
    cout << "Total fault number = " << total_num << endl;
    cout << "Detected fault number = " << detected_num << endl;
    cout << "Undetected fault number = " << abort_num + redundant_num << endl;
    cout << "Abort fault number = " << abort_num << endl;
    cout << "Redundant fault number = " << redundant_num << endl;
    cout << "---------------------------------------" << endl;
    cout << "Total equivalent fault number = " << Flist.size() << endl;
    cout << "Equivalent detected fault number = " << eqv_detected_num << endl;
    cout << "Equivalent undetected fault number = " << eqv_abort_num + eqv_redundant_num << endl;
    cout << "Equivalent abort fault number = " << eqv_abort_num << endl;
    cout << "Equivalent redundant fault number = " << eqv_redundant_num << endl;
    cout << "---------------------------------------" << endl;
    cout << "Fault Coverge = " << 100*detected_num/double(total_num) << "%" << endl;
    cout << "Equivalent FC = " << 100*eqv_detected_num/double(Flist.size()) << "%" << endl;
    cout << "Fault Efficiency = " << 100*detected_num/double(total_num - redundant_num) << "%" << endl;
    cout << "---------------------------------------" << endl;
    return;
}

//run PODEM for target fault
//TRUE: test pattern found
//CONFLICT: no test pattern
//FALSE: abort
ATPG_STATUS CIRCUIT::Podem(FAULT* fptr, unsigned &total_backtrack_num)
{
    unsigned i, backtrack_num(0);
    GATEPTR pi_gptr(0), decision_gptr(0);
    ATPG_STATUS status;

    //set all values as unknown
    for (i = 0;i<Netlist.size();++i) { Netlist[i]->SetValue(X); }
    //mark propagate paths
    MarkPropagateTree(fptr->GetOutputGate());
    //propagate fault free value
    status = SetUniqueImpliedValue(fptr);
    switch (status) {
        case TRUE:
            LogicSim();
            //inject faulty value
            if (FaultEvaluate(fptr)) {
                //forward implication
                ScheduleFanout(fptr->GetOutputGate());
                LogicSim();
            }
            //check if the fault has propagated to PO
            if (!CheckTest()) { status = FALSE; }
            break;
        case CONFLICT:
            status = CONFLICT;
            break;
        case FALSE: break;
    }

    while(backtrack_num < BackTrackLimit && status == FALSE) {
        //search possible PI decision
        pi_gptr = TestPossible(fptr);
        if (pi_gptr) { //decision found
            ScheduleFanout(pi_gptr);
            //push to decision tree
            GateStack.push_back(pi_gptr);
            decision_gptr = pi_gptr;
        }
        else { //backtrack previous decision
            while (!GateStack.empty() && !pi_gptr) {
                //all decision tried (1 and 0)
                if (decision_gptr->GetFlag(ALL_ASSIGNED)) {
                    decision_gptr->ResetFlag(ALL_ASSIGNED);
                    decision_gptr->SetValue(X);
                    ScheduleFanout(decision_gptr);
                    //remove decision from decision tree
                    GateStack.pop_back();
                    decision_gptr = GateStack.back();
                }
                //inverse current decision value
                else {
                    decision_gptr->InverseValue();
                    ScheduleFanout(decision_gptr);
                    decision_gptr->SetFlag(ALL_ASSIGNED);
                    ++backtrack_num;
                    pi_gptr = decision_gptr;
                }
            }
            //no other decision
            if (!pi_gptr) { status = CONFLICT; }
        }
        if (pi_gptr) {
            LogicSim();
            //fault injection
            if(FaultEvaluate(fptr)) {
                //forward implication
                ScheduleFanout(fptr->GetOutputGate());
                LogicSim();
            }
            if (CheckTest()) { status = TRUE; }
        }
    } //end while loop

    //clean ALL_ASSIGNED and MARKED flags
    list<GATEPTR>::iterator gite;
    for (gite = GateStack.begin();gite != GateStack.end();++gite) {
        (*gite)->ResetFlag(ALL_ASSIGNED);
    }
    for (gite = PropagateTree.begin();gite != PropagateTree.end();++gite) {
        (*gite)->ResetFlag(MARKED);
    }

    //clear stacks
    GateStack.clear(); PropagateTree.clear();
    
    //assign true values to PIs
    if (status ==  TRUE) {
		for (i = 0;i<PIlist.size();++i) {
		    switch (PIlist[i]->GetValue()) {
			case S1: break;
			case S0: break;
			case D: PIlist[i]->SetValue(S1); break;
			case B: PIlist[i]->SetValue(S0); break;
			case X: PIlist[i]->SetValue(VALUE(2.0 * rand()/(RAND_MAX + 1.0))); break;
			default: cerr << "Illigal value" << endl; break;
		    }
		}//end for all PI
    } //end status == TRUE

    total_backtrack_num += backtrack_num;
    return status;
}

//inject fault-free value and do simple backward implication
//TRUE: fault can be backward propagated to PI
//CONFLICT: conflict assignment
//FALSE: fault can not be propagated to PI
ATPG_STATUS CIRCUIT::SetUniqueImpliedValue(FAULT* fptr)
{
    register ATPG_STATUS status(FALSE);
    GATEPTR igptr(fptr->GetInputGate());
    //backward implication fault-free value
    switch (BackwardImply(igptr, NotTable[fptr->GetValue()])) {
        case TRUE: status = TRUE; break;
        case CONFLICT: return CONFLICT; break;
        case FALSE: break;
    }
    if (!fptr->Is_Branch()) { return status; }
    //if branch, need to check side inputs of the output gate
    GATEPTR ogptr(fptr->GetOutputGate());
    VALUE ncv(NCV[ogptr->GetFunction()]);
    //set side inputs as non-controlling value
    for (unsigned i = 0;i < ogptr->No_Fanin();++i) {
        if (igptr == ogptr->Fanin(i)) { continue; }
        switch (BackwardImply(ogptr->Fanin(i), ncv)) {
            case TRUE: status = TRUE; break;
            case CONFLICT: return CONFLICT; break;
            case FALSE: break;
        }
    }
    return status;
}

//apply the input values of gate according to its output value
//TRUE: fault can be backward propagated to PI
//CONFLICT: conflict assignment
//FALSE: fault can not be propagated to PI
ATPG_STATUS CIRCUIT::BackwardImply(GATEPTR gptr, VALUE value)
{
    register unsigned i;
    register ATPG_STATUS status(FALSE);
    GATEFUNC fun(gptr->GetFunction());

    if (fun == G_PI) { //reach PI
        //conflict assignment
        if (gptr->GetValue() != X && gptr->GetValue() != value) {
            return CONFLICT;
        }
        gptr->SetValue(value); ScheduleFanout(gptr);
        return TRUE;
    }
    //not PI gate
    switch (fun) {
        case G_NOT:
            switch (BackwardImply(gptr->Fanin(0), NotTable[value])) {
                case TRUE: status = TRUE; break;
                case FALSE: break;
                case CONFLICT: return CONFLICT; break;
            }
            break;
        case G_BUF:
            switch (BackwardImply(gptr->Fanin(0), value)) {
                case TRUE: status = TRUE; break;
                case FALSE: break;
                case CONFLICT: return CONFLICT; break;
            }
            break;
        case G_AND:
        case G_OR:
            if (value != NCV[fun]) { break; }
            for (i = 0;i<gptr->No_Fanin();++i) {
                switch (BackwardImply(gptr->Fanin(i), NCV[fun])) {
                    case TRUE: status = TRUE; break;
                    case FALSE: break;
                    case CONFLICT: return CONFLICT; break;
                }
            }
            break;
        case G_NAND:
        case G_NOR:
            if (value != CV[fun]) { break; }
            for (i = 0;i<gptr->No_Fanin();++i) {
                switch (BackwardImply(gptr->Fanin(i), NCV[fun])) {
                    case TRUE: status = TRUE; break;
                    case FALSE: break;
                    case CONFLICT: return CONFLICT; break;
                }
            }
            break;
        default: break;
    }
    return status;
}

//mark and push propagate tree to stack PropagateTree
void CIRCUIT::MarkPropagateTree(GATEPTR gptr)
{
    PropagateTree.push_back(gptr);
    gptr->SetFlag(MARKED);
    for (unsigned i = 0;i<gptr->No_Fanout();++i) {
        if (gptr->Fanout(i)->GetFlag(MARKED)) { continue; }
        MarkPropagateTree(gptr->Fanout(i));
    }
    return;
}

//fault injection
//true: fault is injected successfully and need to do fault propagation
//false: output value is the same with original one or fault is injected in PO
bool CIRCUIT::FaultEvaluate(FAULT* fptr)
{
    GATEPTR igptr(fptr->GetInputGate());
    //store input value
    VALUE ivalue(igptr->GetValue());
    //can not do fault injection
    if (ivalue == X || ivalue == fptr->GetValue()) { return false; }
    else if (ivalue == S1) { igptr->SetValue(D); }
    else if (ivalue == S0) { igptr->SetValue(B); }
    else { return false; } //fault has been injected

    if (!fptr->Is_Branch()) { return true; }
    //for branch fault, the fault has to be propagated to output gate
    GATEPTR ogptr(fptr->GetOutputGate());
    if (ogptr->GetFunction() == G_PO) { return false; }
    VALUE value(Evaluate(ogptr));
    //backup original value to input gate
    igptr->SetValue(ivalue);
    //fault has propagated to output gate
    if (value != ogptr->GetValue()) {
        ogptr->SetValue(value);
        return true;
    }
    return false;
}

//return possible PI decision
GATEPTR CIRCUIT::TestPossible(FAULT* fptr)
{
    GATEPTR decision_gptr;
    GATEPTR ogptr(fptr->GetOutputGate());
    VALUE decision_value;
    if (!ogptr->GetFlag(OUTPUT)) {
        if (ogptr->GetValue() != X) {
            //if no fault injected, return 0
            if (ogptr->GetValue() != B && ogptr->GetValue() != D) { return 0; }
            //search D-frontier
            decision_gptr = FindPropagateGate();
            if (!decision_gptr) { return 0;}
            switch (decision_gptr->GetFunction()) {
                case G_AND:
                case G_NOR: decision_value = S1; break;
                case G_NAND:
                case G_OR: decision_value = S0; break;
                default: return 0;
            }
        }
        else { //output gate == X
            //test if any unknown path can propagate the fault
            if (!TraceUnknownPath(ogptr)) { return 0; }
            if (!fptr->Is_Branch()) { //stem
                decision_value = NotTable[fptr->GetValue()];
                decision_gptr = ogptr;
            }
            else { //branch
                //output gate value is masked by side inputs
                if (fptr->GetInputGate()->GetValue() != X) {
                    switch (ogptr->GetFunction()) {
                        case G_AND:
                        case G_NOR: decision_value = S1; break;
                        case G_NAND:
                        case G_OR: decision_value = S0; break;
                        default: return 0;
                    }
                    decision_gptr = fptr->GetOutputGate();
                }
                //both input and output values are X
                else {
                    decision_value = NotTable[fptr->GetValue()];
                    decision_gptr = fptr->GetInputGate();
                }
            } //end branch
        } //end output gate == X
    } //end if output gate is PO
    else { //reach PO
        if (fptr->GetInputGate()->GetValue() == X) {
            decision_value = NotTable[fptr->GetValue()];
            decision_gptr = fptr->GetInputGate();
        }
        else { return 0; }
    }
    return FindPIAssignment(decision_gptr, decision_value);
}

//find PI decision to set gptr = value
//success: return PI
//fail: return 0
GATEPTR CIRCUIT::FindPIAssignment(GATEPTR gptr, VALUE value)
{
    //search PI desicion
    if (gptr->GetFunction() == G_PI) {
        gptr->SetValue(value);
        return gptr;
    }
    GATEPTR j_gptr(0); //J-frontier
    VALUE j_value(X), cv_out;
    switch (gptr->GetFunction()) {
        case G_AND:
        case G_NAND:
        case G_OR:
        case G_NOR:
            cv_out = CV[gptr->GetFunction()];
            cv_out = gptr->Is_Inversion()? NotTable[cv_out]: cv_out;
            //select one fanin as cv
            if (value == cv_out) {
                j_gptr = FindEasiestControl(gptr);
                j_value = CV[gptr->GetFunction()];
            }
            //select one fanin as ncv 
            else {
                j_gptr = FindHardestControl(gptr);
                j_value = NCV[gptr->GetFunction()];
            }
            break;
        case G_BUF:
        case G_NOT:
            j_value = gptr->Is_Inversion()? NotTable[value]: value;
            j_gptr = gptr->Fanin(0);
            break;
        default:
            break;
    }
    if (j_gptr) { return FindPIAssignment(j_gptr, j_value); }
    return 0;
}

//check if the fault has propagated to PO
bool CIRCUIT::CheckTest()
{
    VALUE value;
    for (unsigned i = 0;i<POlist.size();++i) {
        value = POlist[i]->GetValue();
        if (value == B || value == D) { return true; }
    }
    return false;
}

//search gate from propagate tree to propagate the fault
GATEPTR CIRCUIT::FindPropagateGate()
{
    register unsigned i;
    list<GATEPTR>::iterator gite;
    GATEPTR gptr, fanin;
    for (gite = PropagateTree.begin();gite!=PropagateTree.end();++gite) {
        gptr = *gite;
        if (gptr->GetValue() != X) { continue; }
        for (i = 0;i<gptr->No_Fanin(); ++i) {
            fanin = gptr->Fanin(i);
            if (fanin->GetValue() != D && fanin->GetValue() != B) { continue; }
            if (TraceUnknownPath(gptr)) { return gptr; }
            break;
        }
    }
    return 0;
}

//trace if any unknown path from gptr to PO
bool CIRCUIT::TraceUnknownPath(GATEPTR gptr)
{
    if (gptr->GetFlag(OUTPUT)) { return true; }
    GATEPTR fanout;
    for (unsigned i = 0;i<gptr->No_Fanout();++i) {
        fanout = gptr->Fanout(i);
        if (fanout->GetValue()!=X) { continue; }
        if (TraceUnknownPath(fanout)) { return true; }
    }
    return false;
}

//serach lowest level unknown fanin
GATEPTR CIRCUIT::FindEasiestControl(GATEPTR gptr)
{
    GATEPTR fanin;
    for (unsigned i = 0;i< gptr->No_Fanin();++i) {
        fanin = gptr->Fanin(i);
        if (fanin->GetValue() == X) { return fanin; }
    }
    return 0;
}

//serach highest level unknown fanin
GATEPTR CIRCUIT::FindHardestControl(GATEPTR gptr)
{
    GATEPTR fanin;
    for (unsigned i = gptr->No_Fanin();i>0;--i) {
        fanin = gptr->Fanin(i-1);
        if (fanin->GetValue() == X) { return fanin; }
    }
    return 0;
}

//functor, used to compare the levels of two gates
struct sort_by_level
{
    bool operator()(const GATEPTR gptr1, const GATEPTR gptr2) const
    {
        return (gptr1->GetLevel() < gptr2->GetLevel());
    }
};

//sort fanin by level (from low to high)
void CIRCUIT::SortFaninByLevel()
{
    for (unsigned i = 0;i<Netlist.size();++i) {
        vector<GATE*> &Input_list = Netlist[i]->GetInput_list();
        sort(Input_list.begin(), Input_list.end(), sort_by_level());
    }
    return;
}

//mark and trace all stem fault propagated by fault "val"
void CIRCUIT::TraceDetectedStemFault(GATEPTR gptr, VALUE val)
{
    //no PO stem fault
    if (gptr->GetFunction() == G_PO) { return; }
    //get output fault type
    if (gptr->Is_Inversion()) { val = NotTable[val]; }
    //translate value to flag: S0->ALL_ASSIGNED, S1->MARKED
    FLAGS flag = FLAGS(val);
    if (val != S1 && val != S0) { cerr << "error" << endl; }
    //stem fault has been detected
    if (gptr->GetFlag(flag)) { return; }
    gptr->SetFlag(flag);
    //stop when the gate meets branch
    if (gptr->No_Fanout() > 1) { return; }
    TraceDetectedStemFault(gptr->Fanout(0), val);
    return;
}

//generate stuck-at fault list using checkpoint theorem
void CIRCUIT::CheckpointFaultList()
{
    cout << "Generate stuck-at fault list using checkpoint theorem." << endl;
    // PIset
    std::unordered_set<GATEPTR> pi_set(PIlist.begin(), PIlist.end());

    GATEPTR gptr, fanout;
    FAULT *fptr;

    // PI stuck at fault
    for (unsigned int z = 0; z < No_PI(); ++z) {
        gptr = PIlist[z];
        // s.a.0
        fptr = new FAULT(gptr, gptr, S0);
        Flist.push_front(fptr);
        // s.a.1
        fptr = new FAULT(gptr, gptr, S1);
        Flist.push_front(fptr);
    }

    // branches stuck at fault
    for (unsigned int i = 0; i < No_Gate(); ++i) {
        gptr = Netlist[i];

        // gate = pi
        if (pi_set.find(gptr) != pi_set.end()) {
            continue;
        }

        // no branch
        if (gptr->No_Fanout() == 1) {
            continue;
        }

        // branch
        for (unsigned int j = 0; j < gptr->No_Fanout(); ++j) {
            fanout = gptr->Fanout(j);
            // s.a.0
            fptr = new FAULT(gptr, fanout, S0);
            fptr->SetBranch(true);
            Flist.push_front(fptr);
            // s.a.1
            fptr = new FAULT(gptr, fanout, S1);
            fptr->SetBranch(true);
            Flist.push_front(fptr);
        }
    }
    UFlist = Flist;

    return;
}

void CIRCUIT::BridgingFaultList()
{
    if (option.retrieve("output")){
        cout << "Generate possible bridging faults list." << endl;
        vector<GATE*> levellist;
        GATEPTR gptr, ngptr;
        FAULT *fptr;
        string outputfilename = GetName() + "_bridging.out";
        ofstream op;
        op.open(outputfilename);        
        op << "These are all possible bridging faults."<<endl;
        //generate bridging faults list
        for(int j=0;j<=MaxLevel;j++){
            // build a list containing gates of same level.
            for (int i = 0;i<No_Gate();i++) {
                if(Gate(i)->GetLevel() == j){
                    levellist.push_back(Gate(i));
                }
            }
            //store and cout the bridging faults based on levelist each time
            for(int i=0;i<levellist.size();i++){
                if(i == (levellist.size()-1)){ break; }
                gptr = levellist[i]; ngptr = levellist[i+1];
                if(gptr->GetLevel() == j){
                    //add bridging fault to BFlist
                    fptr = new FAULT(gptr, ngptr, S0);
                    BFlist.push_front(fptr);          
                    fptr = new FAULT(gptr, ngptr, S1);
                    BFlist.push_front(fptr);                                
                    //cout<<"("<<gptr->GetName()<<", "<<ngptr->GetName()<<", AND) when ("<<gptr->GetName()<<"=0, "<<ngptr->GetName()<<"=1)\n";
                    //cout<<"("<<gptr->GetName()<<", "<<ngptr->GetName()<<", OR) when ("<<gptr->GetName()<<"=1, "<<ngptr->GetName()<<"=0)\n";
                    op <<"("<<gptr->GetName()<<", "<<ngptr->GetName()<<", AND) when ("<<gptr->GetName()<<"=0, "<<ngptr->GetName()<<"=1)\n";
                    op <<"("<<gptr->GetName()<<", "<<ngptr->GetName()<<", OR) when ("<<gptr->GetName()<<"=1, "<<ngptr->GetName()<<"=0)\n";                    
                }            
            }
        }
        UFlist = BFlist;
        op.close();        
    }    
    else{
        cout << "Generate possible bridging faults list." << endl;
        vector<GATE*> levellist;
        GATEPTR gptr, ngptr ;
        FAULT *fptr;
        //generate bridging faults list
        for(int j=0;j<=MaxLevel;j++){
            // build a list containing gates of same level.
            for (int i = 0;i<No_Gate();i++) {
                if(Gate(i)->GetLevel() == j){
                    levellist.push_back(Gate(i));
                }
            }
            //store and cout the bridging faults based on levelist each time
            for(int i=0;i<levellist.size();i++){
                if(i == (levellist.size()-1)){ break; }
                gptr = levellist[i]; ngptr = levellist[i+1];
                if(gptr->GetLevel() == j){
                    //add bridging fault to BFlist
                    fptr = new FAULT(gptr, ngptr, S0);
                    BFlist.push_front(fptr);          
                    fptr = new FAULT(gptr, ngptr, S1);
                    BFlist.push_front(fptr);                                
                }            
            }
        }                     
        UFlist = BFlist;         
    }
    return;
}

//generate all stuck-at fault list
void CIRCUIT::GenerateSpeFaultList()
{
    cout << "Generate Specified list" << endl;
    string f1 = "net17";
    string f2 = "n60";
    register unsigned i, j;
    GATEFUNC fun;
    GATEPTR gptr, fanout;
    FAULT *fptr;
    for (i = 0;i<No_Gate();++i) {
        gptr = Netlist[i]; 
        fun = gptr->GetFunction();
        //cout << "Name = "<<gptr->GetName()<< endl;
        if (fun == G_PO) { continue; } //skip PO
        else if(gptr->GetName()!=f1&&gptr->GetName()!=f2) { continue; }
        if(gptr->GetName()==f1){
            //add stem stuck-at 0 fault to Flist
            fptr = new FAULT(gptr, gptr, S0);
            Flist.push_front(fptr);
            cout << "Add "<<gptr->GetName()<<" stuck at 0 to Fault list"<< endl;
        }
        else if(gptr->GetName()==f2){
            //add stem stuck-at 1 fault to Flist
            fptr = new FAULT(gptr, gptr, S1);
            Flist.push_front(fptr);            
            cout << "Add "<<gptr->GetName()<<" stuck at 1 to Fault list"<< endl;
        }
    } //end all gates
    //copy Flist to undetected Flist (for fault simulation)
    UFlist = Flist;  
    return;
}

// PODEM ATPG with tracing (fault dropping)
void CIRCUIT::AtpgWithTrace()
{
    cout << "Run stuck-at fault ATPG with Trace" << endl;
    unsigned i, total_backtrack_num(0), pattern_num(0);
    ATPG_STATUS status;
    FAULT* fptr;
    list<FAULT*>::iterator fite;
    
    // Prepare the output files
    ofstream OutputStrm;
    if (option.retrieve("output")){
        OutputStrm.open((char*)option.retrieve("output"), ios::out);
        if(!OutputStrm){
            cout << "Unable to open output file: "
                 << option.retrieve("output") << endl;
            cout << "Unsaved output!\n";
            exit(-1);
        }
    }

    if (option.retrieve("output")){
        for (i = 0; i < PIlist.size(); ++i) {
            OutputStrm << "PI " << PIlist[i]->GetName() << " ";
        }
        OutputStrm << endl;
    }

    for (fite = Flist.begin(); fite != Flist.end(); ++fite) {
        fptr = *fite;

        // print fault in process
        cout << "Processing fault at node " << fptr->GetOutputGate()->GetName()
             << " with stuck-at value " << fptr->GetValue() << endl;

        // Run PODEM algorithm with trace
        status = PodemWithTrace(fptr, total_backtrack_num);

        switch (status) {
            case TRUE:
                fptr->SetStatus(DETECTED);
                ++pattern_num;
                // Print test pattern
                cout << "Test pattern found for fault at node "
                     << fptr->GetOutputGate()->GetName() << ": ";
                for (i = 0; i < PIlist.size(); ++i) {
                    cout << PIlist[i]->GetName() << "=" << PIlist[i]->GetValue() << " ";
                    ScheduleFanout(PIlist[i]); 
                    if (option.retrieve("output")){ OutputStrm << PIlist[i]->GetValue(); }
                }
                cout << endl;
                if (option.retrieve("output")){ OutputStrm << endl; }
                for (i = PIlist.size(); i < Netlist.size(); ++i) { Netlist[i]->SetValue(X); }
                LogicSim();
                FaultSim();
                break;
            case CONFLICT:
                fptr->SetStatus(REDUNDANT);
                cout << "Fault at node " << fptr->GetOutputGate()->GetName()
                     << " is redundant." << endl;
                break;
            case FALSE:
                fptr->SetStatus(ABORT);
                cout << "Abort processing fault at node "
                     << fptr->GetOutputGate()->GetName() << endl;
                break;
        }
        cout << "--------------------------------------------------------"<<endl;
    } // end all faults

}

// Run PODEM for target fault with tracing
ATPG_STATUS CIRCUIT::PodemWithTrace(FAULT* fptr, unsigned &total_backtrack_num)
{
    unsigned i, backtrack_num(0);
    GATEPTR pi_gptr(0), decision_gptr(0);
    ATPG_STATUS status;

    // Set all values as unknown
    for (i = 0; i < Netlist.size(); ++i) { Netlist[i]->SetValue(X); }

    // Mark propagate paths
    MarkPropagateTree(fptr->GetOutputGate());

    // print fault activation
    cout << "Activating fault at node " << fptr->GetOutputGate()->GetName()
         << " with stuck-at value " << fptr->GetValue() << endl;

    // Propagate fault-free value
    status = SetUniqueImpliedValue(fptr);
    switch (status) {
        case TRUE:
            LogicSim();
            // Inject faulty value
            if (FaultEvaluate(fptr)) {
                // Forward implication
                ScheduleFanout(fptr->GetOutputGate());
                LogicSim();
            }
            // Check if the fault has propagated to PO
            if (!CheckTest()) { status = FALSE; }
            break;
        case CONFLICT:
            status = CONFLICT;
            cout << "Conflict detected during unique path sensitization." << endl;
            break;
        case FALSE:
            break;
    }

    while (backtrack_num < BackTrackLimit && status == FALSE) {
        // Search possible PI decision
        pi_gptr = TestPossible(fptr);
        if (pi_gptr) { // Decision found
            // print dicision
            cout << "Decision made: Assigning value " << pi_gptr->GetValue()
                 << " to PI node " << pi_gptr->GetName() << endl;

            ScheduleFanout(pi_gptr);
            // Push to decision tree
            GateStack.push_back(pi_gptr);
            decision_gptr = pi_gptr;
        }
        else { // Backtrack previous decision
            cout << "No further decisions possible, backtracking..." << endl;
            while (!GateStack.empty() && !pi_gptr) {
                decision_gptr = GateStack.back();
                // All decisions tried (1 and 0)
                if (decision_gptr->GetFlag(ALL_ASSIGNED)) {
                    cout << "All assignments tried for node " << decision_gptr->GetName()
                         << ", undoing decision." << endl;
                    decision_gptr->ResetFlag(ALL_ASSIGNED);
                    decision_gptr->SetValue(X);
                    ScheduleFanout(decision_gptr);
                    // Remove decision from decision tree
                    GateStack.pop_back();
                }
                // Inverse current decision value
                else {
                    decision_gptr->InverseValue();
                    ScheduleFanout(decision_gptr);
                    decision_gptr->SetFlag(ALL_ASSIGNED);
                    ++backtrack_num;
                    cout << "Backtracked: Assigning inverse value "
                         << decision_gptr->GetValue() << " to node "
                         << decision_gptr->GetName() << endl;
                    pi_gptr = decision_gptr;
                }
            }
            // No other decision
            if (!pi_gptr) {
                status = CONFLICT;
                cout << "Backtracking failed, conflict encountered." << endl;
            }
        }
        if (pi_gptr) {
            LogicSim();
            // Fault injection
            if (FaultEvaluate(fptr)) {
                // Forward implication
                ScheduleFanout(fptr->GetOutputGate());
                LogicSim();
            }
            if (CheckTest()) {
                status = TRUE;
                cout << "Fault effect has been propagated to PO, test pattern found." << endl;
            }
        }
    } // end while loop

    if (backtrack_num >= BackTrackLimit) {
        cout << "Backtrack limit reached (" << BackTrackLimit << "), aborting." << endl;
        status = FALSE;
    }

    // Clean ALL_ASSIGNED and MARKED flags
    list<GATEPTR>::iterator gite;
    for (gite = GateStack.begin(); gite != GateStack.end(); ++gite) {
        (*gite)->ResetFlag(ALL_ASSIGNED);
    }
    for (gite = PropagateTree.begin(); gite != PropagateTree.end(); ++gite) {
        (*gite)->ResetFlag(MARKED);
    }

    // Clear stacks
    GateStack.clear();
    PropagateTree.clear();
    
    // Assign true values to PIs
    if (status == TRUE) {
        cout << "Final test pattern: ";
        for (i = 0; i < PIlist.size(); ++i) {
            switch (PIlist[i]->GetValue()) {
                case S1:
                    cout << PIlist[i]->GetName() << "=1 ";
                    break;
                case S0:
                    cout << PIlist[i]->GetName() << "=0 ";
                    break;
                case D:
                    PIlist[i]->SetValue(S1);
                    cout << PIlist[i]->GetName() << "=1 ";
                    break;
                case B:
                    PIlist[i]->SetValue(S0);
                    cout << PIlist[i]->GetName() << "=0 ";
                    break;
                case X:
                    PIlist[i]->SetValue(VALUE(2.0 * rand()/(RAND_MAX + 1.0)));
                    cout << PIlist[i]->GetName() << "=X ";
                    break;
                default:
                    cerr << "Illegal value" << endl;
                    break;
            }
        } // end for all PI
        cout << endl;
    } // end status == TRUE

    total_backtrack_num += backtrack_num;
    return status;
}

// Assignment 6D integrate random and PODEM
void CIRCUIT::Ass6d()
{
    cout << "\n*************Run Random Pattern and Simulate**********\n" << endl;
    int pat =0;
    float coverage=0.0;   
    unsigned i;
    unsigned seed = static_cast<unsigned>(time(0)); // Use current time as the seed
    srand(seed); // Set seed
    //Prepare the output files
    ofstream OutputStrm;

    if (option.retrieve("output")){
        OutputStrm.open((char*)option.retrieve("output"),ios::out);
        if(!OutputStrm){
              cout << "Unable to open output file: "
                   << option.retrieve("output") << endl;
              cout << "Unsaved output!\n";
              exit(-1);
        }
    }
    if (option.retrieve("output")){
	    for (i = 0;i<PIlist.size();++i) {
		OutputStrm << "PI " << PIlist[i]->GetName() << " ";
	    }
	    OutputStrm << endl;
    }
      
    while(pat<999 && coverage<90){
        string piname;      
        // Generate one random pattern
        for(i = 0;i<PIlist.size();++i){
            int ra;
            ra = (1 + (int)(10.0 * rand() / (RAND_MAX + 1.0)))%2 ;
            if (option.retrieve("output")){ 
                OutputStrm << ra;
            }            
        }        
        OutputStrm << endl;
        // OutputStrm.close();
        // InitPattern
        InitPattern(const_cast<char *>(option.retrieve("output")));
        //Faultsimvectors
        unsigned pattern_num(0);
        if(!Pattern.eof()){ // Readin the first vector
            while(!Pattern.eof()){
                ++pattern_num;
                Pattern.ReadNextPattern();
                //fault-free simulation
                SchedulePI();
                LogicSim();
                //single pattern parallel fault simulation
                FaultSim();
            }
        }

        //compute fault coverage
        unsigned total_num(0);
        unsigned undetected_num(0), detected_num(0);
        unsigned eqv_undetected_num(0), eqv_detected_num(0);
        FAULT* fptr;
        list<FAULT*>::iterator fite;
        for (fite = Flist.begin();fite!=Flist.end();++fite) {
            fptr = *fite;
            switch (fptr->GetStatus()) {
                case DETECTED:
                    ++eqv_detected_num;
                    detected_num += fptr->GetEqvFaultNum();
                    break;
                default:
                    ++eqv_undetected_num;
                    undetected_num += fptr->GetEqvFaultNum();
                    break;
            }
        }
        total_num = detected_num + undetected_num;
        coverage = 100*detected_num/double(total_num);   
        pat += 1;     
        Pattern.close();          
        if(pat>999 || coverage>90){
            cout.setf(ios::fixed);
            cout.precision(2);
            cout << "Test pattern number= " << pat << endl;                        
            cout << "---------------------------------------" << endl;
            cout << "Total fault number = " << total_num << endl;
            cout << "Detected fault number = " << detected_num << endl;
            cout << "Undetected fault number = " << undetected_num << endl;
            cout << "---------------------------------------" << endl;
            cout << "Equivalent fault number = " << Flist.size() << endl;
            cout << "Equivalent detected fault number = " << eqv_detected_num << endl; 
            cout << "Equivalent undetected fault number = " << eqv_undetected_num << endl; 
            cout << "---------------------------------------" << endl;
            cout << "Fault Coverge = " << 100*detected_num/double(total_num) << "%" << endl;
            cout << "Equivalent FC = " << 100*eqv_detected_num/double(Flist.size()) << "%" << endl;
            cout << "---------------------------------------" << endl;
            cout << "coverage : "<<coverage<<endl ;   
        }
    }          
}
