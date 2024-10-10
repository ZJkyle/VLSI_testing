#ifndef READPATTERN_H
#define READPATTERN_H

#include <fstream>
#include "gate.h"
using namespace std;

class PATTERN
{
    private:
        ifstream patterninput;
        vector<GATE*> inlist;
        int no_pi_infile;
    public:
        PATTERN(): no_pi_infile(0){}
        void Initialize(char* InFileName, int no_pi, string TAG);
        //Assign next input pattern to PI
        void ReadNextPattern();
        void ReadNextPattern_t();
	void ReadNextPattern(unsigned idx);
        bool eof()
        {
            return (patterninput.eof());
        }
        // Function to close patterninput file stream
        void ClosePatternFile()
        {
            if (patterninput.is_open()) {
                patterninput.close();
                cout << "Pattern input file closed successfully." << endl;
            } else {
                cout << "Pattern input file was not open." << endl;
            }
        }
        // Reset function to clear previous patterns
        void ResetPattern()
        {
            inlist.clear();    // 清空輸入 pattern 的列表
            no_pi_infile = 0;  // 重置計數器
        }
        // VLSI-Testing lab3
		vector<GATE*>* getInlistPtr() { return &inlist;}
};
#endif
