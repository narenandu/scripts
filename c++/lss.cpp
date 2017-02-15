#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <algorithm>
#include <dirent.h>
#include <errno.h>

using namespace std;

class Data
{
    public:
        vector<int> start;
        vector<int> end;
        int length;
        int count;

        Data();
        Data(const Data&);
        Data(int, int, vector<int>, vector<int>);           
        ~Data(){};
};

Data::Data()                //Constructor
{
    start = vector<int> (0);
    end = vector<int> (0);
    length = 0;
    count = 0;
}

Data::Data(const Data &c)   //Copy Constructor
{
    start = c.start;
    end = c.end;
    length = c.length;
    count = c.count;
}

Data::Data(int count, int length, vector<int> start, vector<int> end)                //Constructor
{
    this->start = start;
    this->end = end;
    this->length = length;
    this->count = count;
}

typedef pair<string, Data> myPair;

int getFiles(string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name,".") && strcmp(dirp->d_name, ".."))
            files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

int main(int argc, char *argv[])
{
    // Getting files in the folder
    string dir;

    if (argc == 1)
        dir = string(".");
    else if (argc > 2)
    {
        cout<<"INFO : Supporting only one argument at the moment, please pass only one"<<endl;
        return 0;
    }
    else 
        dir = string(argv[1]);

    vector<string> files = vector<string>();
    getFiles(dir,files);

    // sorting the files
    sort(files.begin(), files.end());

    // using map as a stack to push the files as they come
    vector<myPair> data;
    string pattern;

    // loop over the fileList
    for (unsigned int i = 0;i < files.size();i++) 
    {
        set<string> fParts, lastFParts;
        smatch m;
        regex nonNumeric ("\\D+");   
        regex numeric ("\\d+");
        
        string f = files[i];
        string tempF = f;
        // parts of the file based on non-numeric types
        while(regex_search(tempF,m,nonNumeric))
        {
            for (auto x:m) fParts.insert(x);
            tempF = m.suffix().str();
        }
        tempF = f;        
        // parts of the file based on numeric types
        while(regex_search(tempF,m,numeric))
        {
            for (auto x:m) fParts.insert(x);
            tempF = m.suffix().str();
        }   

        if (data.empty() != 1)
        {
            //Get the last file entry in the Map
            myPair lastPair = *(data.rbegin());
            string lastF = lastPair.first;
            tempF = lastF;

            // parts of the file based on non-numeric types
            while(regex_search(tempF,m,nonNumeric))
            {
                for (auto x:m) lastFParts.insert(x);
                tempF = m.suffix().str();
            }
            tempF = lastF;        
            // parts of the file based on numeric types
            while(regex_search(tempF,m,numeric))
            {
                for (auto x:m) lastFParts.insert(x);
                tempF = m.suffix().str();
            }              
            // most of the EFFICIENCY comes from here since we might find the pattern already recorded in case of a large number of files with in the sequence
            // Also checking if previous file is not a substring of current file because it is a positive RE match (lastF not in f)
            // Remote possibility of finding a positive RE match where X.Y can be matched to X<anything>Y at the start of file name
            bool matchingPattern = regex_match(f, m, regex(lastF));
            string dotCheck = lastF;
            dotCheck[dotCheck.find('.')] = '_';

            if (matchingPattern && f.find(lastF) == string::npos && f.find(dotCheck) == string::npos)
            {

                //pop the last entry to update the values further down and push it back again
                Data value = lastPair.second;
                data.pop_back();

                int count = value.count + 1;
                string number = m[1];
                int endNumber = *(value.end.rbegin());
                vector<int> start = {value.start};
                vector<int> end = {value.end};
                int length = value.length;
                int num = stoi(number);


                // Break found in the sequence, record another start and end number
                if ((num - endNumber) > 1)
                {
                    start.push_back(num);
                    end.push_back(num);
                }
                else // continuous numbers in pattern, update the sequence range end
                {
                    *(end.rbegin()) = endNumber+1; 
                }
                // construct the dictionary with all the variables
                Data temp (count, length, start, end);
                myPair tempPair = make_pair(lastF, temp);
                data.push_back(tempPair);
                continue;
            }
        }
  
        // Diff the set of file parts to know exactly the varying part of file name  
        set<string> diff;
        set_difference(fParts.begin(), fParts.end(), 
                       lastFParts.begin(), lastFParts.end(),
                       inserter(diff, diff.end()));

        string diffStr = *(diff.begin());
        myPair tempPair;
        // if a valid sequence was found
        if (diff.size()==1 && isdigit(diffStr[0]) && lastFParts.size()>1 && fParts.size()>1)
        {
            // pop the last entry in stack
            myPair lastPair = *(data.rbegin());
            Data lastEntry = lastPair.second;
            data.pop_back();

            int number = stoi(diffStr);
            int length = diffStr.size();
            string delimiter = diffStr;
            string token1 = f.substr(0, f.find(delimiter));
            string token2 = f.substr(f.find(delimiter)+delimiter.size(), f.size());
            string pattern = token1 + "(\\d{" + to_string(length) + "})" + token2;

            // construct the dictionary with all the variables
            Data temp (2, length, vector<int> {number-1}, vector<int> {number});
            tempPair = make_pair(pattern, temp);
        }
        else
        {
            Data temp;
            temp.count = -1;
            tempPair = make_pair(f, temp);
        }
        data.push_back(tempPair);      // push the entry on to stack
    }

    // Finally print out the required file List as the output of lss
    for (vector<myPair>::iterator it=data.begin(); it!=data.end(); ++it)
    {
        myPair lastPair = *(it);
        string k = lastPair.first;
        Data v = lastPair.second;
        string line;
        // if a single entry is found
        if (v.count == -1)
        {
            line = "1 " + k;
        }
        else             
        {   
            // replace the regex pattern with c-style pattern for display purpose
            string filePattern(k);
            string toFind1 = "(\\d{";
            string toFind2 = "})";

            if (v.length == 1)
                filePattern = k.replace(k.find("(\\d{1"), 5, "%");
            else
                filePattern = k.replace(k.find(toFind1), toFind1.length(), "%0");
            filePattern = k.replace(k.find(toFind2), toFind2.length(), "d");

            // if the file name is a not a continuous sequence, but with breaks in it, print here   
            if(v.start.size() > 1)        
            {
                line = to_string(v.count) + " " + filePattern + " ";
                for (unsigned int i=0; i < v.start.size(); ++i)
                {
                    line += to_string(v.start[i]) + "-" + to_string(v.end[i]) + " ";
                }
            }

            //if the file name is a continuous sequence, print it here
            else 
            {
                line = to_string(v.count) + " " + filePattern + " " + to_string(v.start[0]) + "-" + to_string(v.end[0]);
            }
        }
        cout<<line<<endl;
    }
    return 0;
}
