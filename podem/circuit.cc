#include <iostream> 
//#include <alg.h>
#include "circuit.h"
#include "GetLongOpt.h"
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
    // # of branch nets
    // # of stem nets
    // Avg # of fanouts of each gate
    // int num_nets = 0, num_bran = 0, num_stem = 0;
    // int temp=0, tempbr=0;
    // float out = 0, avgout = 0;
    // for(int i = 0; i < Circuit.No_Gate();i++){
    //     cout << Circuit.Gate(i)-> GetName() << "'s func: " << Circuit.Gate(i)->GetFunction()<<endl;
    //     if (Circuit.Gate(i)->No_Fanout() >= 2){
    //         temp = Circuit.Gate(i)->No_Fanout() +1;
    //         tempbr = Circuit.Gate(i)->No_Fanout();
    //         num_stem += 1;
    //     }
    //     else{
    //         temp = Circuit.Gate(i)->No_Fanout();
    //         tempbr = 0;
    //     }
    //     num_bran += tempbr;
    //     num_nets = num_nets + temp;
    //     out += Circuit.Gate(i)->No_Fanout();
    // }

    // avgout = out / Circuit.No_Gate();
    // cout <<  "Total number of signal nets: " <<  num_nets << endl;
    // cout <<  "Number of branch nets: " << num_bran <<endl;
    // cout <<  "Number of stem nets: " << num_stem <<endl;
    // cout <<  "Average number of fanouts of each gate: "  << avgout <<endl ;
}