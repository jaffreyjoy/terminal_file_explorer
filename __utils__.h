#include <cstring>
#include <string>
#include <algorithm>
#include <filesystem> // for ostringstream
#include <stdlib.h>

using namespace std;


// defines
#define umap_s_s unordered_map<string,string>

#define ALL_PERM 0777
#define MY_READ_PERM 0
#define OTHER_READ_PERM 6
#define MY_WRITE_PERM 1
#define OTHER_WRITE_PERM 7
#define GRP_READ_PERM 3


string path_ka_jugaad_kar(string curr_dir, string gaepathig){
    char* nongaepath = new char[PATH_MAX];
    if(gaepathig[0] == '/'){
        realpath(gaepathig.c_str(), nongaepath);
        string nongaepath_s = string(nongaepath);
        free(nongaepath);
        return nongaepath_s;
    }
    else if (gaepathig[0] == '~'){
        string home_path = string(getenv("HOME"));
        if(gaepathig == "~"){
            return home_path;
        }
        else{
            string home_appended_path =  home_path + "/" + gaepathig.substr(1);
            realpath(home_appended_path.c_str(), nongaepath);
            string nongaepath_s = string(nongaepath);
            free(nongaepath);
            return nongaepath_s;
        }
    }
    string lilgaepath = curr_dir + "/" + gaepathig;
    realpath(lilgaepath.c_str(), nongaepath);
    string nongaepath_s = string(nongaepath);
    delete[] nongaepath;
    return string(nongaepath_s);
}


string trim_newline(string str){
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.cend());
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.cend());
    return str;
}

bool if_permission_granted(umap_s_s& content){
    if(content["perm"][OTHER_READ_PERM]=='r')
        return true;
    else if(getenv("USERNAME")==content["uown"] && content["perm"][MY_READ_PERM]=='r')
        return true;
    else
        return false;
}

void nuke_stack_contents(stack<string>& stk){
    while(!stk.empty())
        stk.pop();
}

int _min(int a, int b){
    return (a<b)?a:b;
}

int _max(int a, int b){
    return (a>b)?a:b;
}

string get_prev_directory_from_path(string path){
    int slash_index;
    int path_ind = 0;
    int path_len = path.length();
    if (count(path.begin(), path.end(), '/') <= 1) return "/";
    else{
        reverse(path.begin(), path.end());
        for(int path_ind=0; path_ind<path_len; path_ind++){
            if( path[path_ind] == '/'){
                slash_index = path_ind;
                break;
            }
        }
    }
    string parent_dir = path.substr(slash_index+1);
    reverse(parent_dir.begin(), parent_dir.end());
    return parent_dir;
}

string get_filename_from_path(string path){
    int file_dir_length =  get_prev_directory_from_path(path).length();
    return path.substr(file_dir_length);
}

string get_contentname_from_path(string path){
    int file_dir_length =  get_prev_directory_from_path(path).length();
    return path.substr(file_dir_length);
}

string get_content_icon(string content_type){
    if(content_type == "file")
        return "📄";
    else if (content_type == "dir")
        return "📁";
    else if (content_type == "slink")
        return "🔗";
    else
        return "💩";
}

bool cmp_content_name(umap_s_s c1, umap_s_s c2){
    return c1["name"] < c2["name"];
}

void cls(){
    cout << "\033[2J\033[1;1H";
}

char* s_to_carr(const string& s) {
    char* carr = new char[s.length()+1];
    strcpy(carr, s.c_str());
    return carr;
}

string convert_to_string(double num) {
    ostringstream convert;
    convert << num;
    return convert.str();
}

double round_off(double n) {
    double d = n * 10.0;
    int i = d + 0.5;
    d = (float)i / 10.0;
    return d;
}

