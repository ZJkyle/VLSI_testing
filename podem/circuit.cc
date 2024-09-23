#include <iostream> 
//#include <alg.h>
#include "circuit.h"
#include "GetLongOpt.h"
#include <unordered_set>

using namespace std;

extern GetLongOpt option;

void CIRCUIT::FanoutList()
{
    unsigned i = 0, j;
    GATE* gptr;
    for (;i < No_Gate();i++) {
        gptr = Gate(i);
        for (j = 0;j < gptr->No_Fanin();j++) {
            gptr->Fanin(j)->AddOutput_list(gptr);
        }
    }
}

void CIRCUIT::Levelize()
{
    list<GATE*> Queue;
    GATE* gptr;
    GATE* out;
    unsigned j = 0;
    for (unsigned i = 0;i < No_PI();i++) {
        gptr = PIGate(i);
        gptr->SetLevel(0);
        for (j = 0;j < gptr->No_Fanout();j++) {
            out = gptr->Fanout(j);
            if (out->GetFunction() != G_PPI) {
                out->IncCount();
                if (out->GetCount() == out->No_Fanin()) {
                    out->SetLevel(1);
                    Queue.push_back(out);
                }
            }
        }
    }
    for (unsigned i = 0;i < No_PPI();i++) {
        gptr = PPIGate(i);
        gptr->SetLevel(0);
        for (j = 0;j < gptr->No_Fanout();j++) {
            out = gptr->Fanout(j);
            if (out->GetFunction() != G_PPI) {
                out->IncCount();
                if (out->GetCount() ==
                        out->No_Fanin()) {
                    out->SetLevel(1);
                    Queue.push_back(out);
                }
            }
        }
    }
    int l1, l2;
    while (!Queue.empty()) {
        gptr = Queue.front();
        Queue.pop_front();
        l2 = gptr->GetLevel();
        for (j = 0;j < gptr->No_Fanout();j++) {
            out = gptr->Fanout(j);
            if (out->GetFunction() != G_PPI) {
                l1 = out->GetLevel();
                if (l1 <= l2)
                    out->SetLevel(l2 + 1);
                out->IncCount();
                if (out->GetCount() ==
                        out->No_Fanin()) {
                    Queue.push_back(out);
                }
            }
        }
    }
    for (unsigned i = 0;i < No_Gate();i++) {
        Gate(i)->ResetCount();
    }
}

void CIRCUIT::Check_Levelization()
{

    GATE* gptr;
    GATE* in;
    unsigned i, j;
    for (i = 0;i < No_Gate();i++) {
        gptr = Gate(i);
        if (gptr->GetFunction() == G_PI) {
            if (gptr->GetLevel() != 0) {
                cout << "Wrong Level for PI : " <<
                gptr->GetName() << endl;
                exit( -1);
            }
        }
        else if (gptr->GetFunction() == G_PPI) {
            if (gptr->GetLevel() != 0) {
                cout << "Wrong Level for PPI : " <<
                gptr->GetName() << endl;
                exit( -1);
            }
        }
        else {
            for (j = 0;j < gptr->No_Fanin();j++) {
                in = gptr->Fanin(j);
                if (in->GetLevel() >= gptr->GetLevel()) {
                    cout << "Wrong Level for: " <<
                    gptr->GetName() << '\t' <<
                    gptr->GetID() << '\t' <<
                    gptr->GetLevel() <<
                    " with fanin " <<
                    in->GetName() << '\t' <<
                    in->GetID() << '\t' <<
                    in->GetLevel() <<
                    endl;
                }
            }
        }
    }
}

void CIRCUIT::SetMaxLevel()
{
    for (unsigned i = 0;i < No_Gate();i++) {
        if (Gate(i)->GetLevel() > MaxLevel) {
            MaxLevel = Gate(i)->GetLevel();
        }
    }
}

//Setup the Gate ID and Inversion
//Setup the list of PI PPI PO PPO
void CIRCUIT::SetupIO_ID()
{
    unsigned i = 0;
    GATE* gptr;
    vector<GATE*>::iterator Circuit_ite = Netlist.begin();
    for (; Circuit_ite != Netlist.end();Circuit_ite++, i++) {
        gptr = (*Circuit_ite);
        gptr->SetID(i);
        switch (gptr->GetFunction()) {
            case G_PI: PIlist.push_back(gptr);
                break;
            case G_PO: POlist.push_back(gptr);
                break;
            case G_PPI: PPIlist.push_back(gptr);
                break;
            case G_PPO: PPOlist.push_back(gptr);
                break;
            case G_NOT: gptr->SetInversion();
                break;
            case G_NAND: gptr->SetInversion();
                break;
            case G_NOR: gptr->SetInversion();
                break;
            default:
                break;
        }
    }
}

// Lab 0 
// Print circuit info 
void CIRCUIT::printINFO()
{
    cout <<  "Circuit name: " << GetName() << endl;
    cout <<  "Number of inputs: " << No_PI() << endl;
    cout <<  "Number of outputs: " << No_PO() << endl;

    // GATEFUNC {G_PI, G_PO, G_PPI, G_PPO, G_NOT, G_AND, G_NAND, G_OR, G_NOR, G_DFF, G_BUF, G_BAD };
    // Calculate # of each gate function.
    int INV=0, AND=0, NAND=0, BUF=0, NOR=0, OR = 0,DFF=0;
    for(int i = 0; i < No_Gate();i++){
        
        // in s713.bench, there are 35 inputs and 19 dff and 23 outputs
        // but i got 35 + 19 = 54 BUF and 23 + 19 = 42 outputs 
        // which means DFF will be shown as 0 and 1 at the same time
        if (Gate(i)->GetFunction() == 4){
            INV = INV + 1;
        }
        else if (Gate(i)->GetFunction() == 5){
            AND = AND + 1;
        }
        else if (Gate(i)->GetFunction() == 6){
            NAND = NAND + 1;
        }
        else if (Gate(i)->GetFunction() == 7){
            OR = OR + 1;
        }
        else if (Gate(i)->GetFunction() == 8){
            NOR = NOR + 1;
        }
        else if (Gate(i)->GetFunction() == 2){
            DFF = DFF + 1;
        }
        
    }

    cout <<  "Total number of gates: " << NOR+OR+AND+NAND+INV << endl;
    cout <<  "Number of gates per type" <<endl;
    cout << "Number of G_INV: " << INV <<"      " << "Number of G_AND: " << AND <<"      " << "Number of G_NAND: "<< NAND <<endl;
    cout << "Number of G_NOR: " << NOR <<"      " << "Number of G_OR: "  << OR <<endl;
    // number of DFF = PPI in here
    cout << "Number of flip-flops: "<< DFF <<endl;
    // Total number of signal nets:
    int total_nets = 0;

    for(int i = 0; i < No_Gate(); i++) {
        int fanout_count = Gate(i)->No_Fanout();
        
        // 如果 fanout > 1，則計算 stem net + branch nets
        if(fanout_count > 1) {
            total_nets += 1 + fanout_count; // 1 個 stem net，加上 fanout_count 個 branch nets
        }
        // 如果 fanout == 1，只計算 1 條 net
        else if(fanout_count == 1) {
            total_nets += 1;
        }
    }

    cout << "Total number of signal nets: " << total_nets << endl;
    // # of branch nets
    int total_branch_nets = 0;
    for(int i = 0; i < No_Gate(); i++) {
        int fanout_count = Gate(i)->No_Fanout();
        // 如果 fanout > 1，計算 branch nets
        if(fanout_count > 1) {
            total_branch_nets += fanout_count; 
        }
    }
    cout << "Total number of branch nets: " << total_branch_nets << endl;    

    // # of stem nets
    int total_stem_nets = 0;

    for(int i = 0; i < No_Gate(); i++) {
        int fanout_count = Gate(i)->No_Fanout();
        // 如果 fanout > 1，計算 stem net
        if(fanout_count > 1) {
            total_stem_nets += 1; // 每個有 fanout 的 gate 有 1 個 stem net
        }
    }

    // Avg # of fanouts of each gate
    int total_fanouts = 0; 
    int total_gates = No_Gate(); 

    for(int i = 0; i < total_gates; i++) {
        int fanout_count = Gate(i)->No_Fanout();
        total_fanouts += fanout_count; // 將每個 gate 的 fanout 累加
    }

    // 計算平均 fanout 數量
    float average_fanouts = static_cast<float>(total_fanouts) / total_gates;
    cout << "Total fanouts: "<<total_fanouts<<endl;
    cout << "Total gates: "<<total_gates<<endl;
    cout << "Average number of fanouts per gate: " << average_fanouts << endl;
}

void CIRCUIT::findpath(const char* startGatename, const char* endGatename){
    // find start/end gate if exist
    vector<GATE*>::iterator it; // PIlist/POlist defined as vector<GATE*>
    GATE *start_gate = nullptr;
    GATE *end_gate = nullptr;

    for(it = PIlist.begin(); it!=PIlist.end(); ++it){
        if((*it)->GetName() == startGatename){
            start_gate = (*it); 
            break;
        }
    }
    if (start_gate == nullptr){
        cerr << "Can't find the Start gate" << endl;
    }

    for(it = POlist.begin(); it!=POlist.end(); ++it){
        if((*it)->GetName() == endGatename){
            end_gate = (*it); 
            break;
        }
    }
    if (end_gate == nullptr){
        cerr << "Can't find the End gate" << endl;
    }
    
    // 使用 DFS 查找所有從起點到終點的路徑
    vector<GATE*> path;
    path.clear();
    int path_count = 0;
    dfs(start_gate, end_gate, path, path_count);

    // 輸出結果
    if (path_count == 0) {
        cout << "No paths found from " << startGatename << " to " << endGatename << endl;
    } else {
        cout << "The paths from " << startGatename << " to " << endGatename << ": " << path_count << endl;
    }

}


bool CIRCUIT::dfs(GATE* current, GATE* destination, vector<GATE*>& path, int& path_count) {
    bool find_path = false;
    int current_fanout_num;
    path.push_back(current);
    GATE *next;

    // Find path = true
    if (current == destination) {
        printPath(path);
        path_count++;
        path.pop_back();
        return true;
    } 
    
    current_fanout_num = current->No_Fanout();
    
    for(int i =0; i < current_fanout_num; ++i){
        bool temp;
        next = current->Fanout(i);

        if(next->getSearchState() != VISITED){
            //path.push_back(next);
            temp = dfs(next, destination, path, path_count);
            if(find_path ==false){
                find_path = temp;
            }
        }
    }
    
    if(find_path == false){
        current->SetSearchState(VISITED);
    }
    path.pop_back();

    return find_path;
}

void CIRCUIT::printPath(vector<GATE*>& path) {
    cout <<endl;
    for (GATE* gate : path) {
        cout << gate->GetName() << " ";
    }
    cout << endl;
}

