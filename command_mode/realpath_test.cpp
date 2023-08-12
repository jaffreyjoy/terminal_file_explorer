#include<iostream>
#include<stdlib.h>

using namespace std;

int main()
{
    string gaepath = "/home/.//./jaff rey/../amit/";
    cout << gaepath << endl;
    char* nongaepath = new char[gaepath.length()*2];
    realpath(gaepath.c_str(), nongaepath);
    cout << string(nongaepath) << endl;
    delete[] nongaepath;
    return 0;
}


