#include <libintl.h>
#include <conio.h>
#include <iostream>
#include <fstream>

// os utils
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>

// other utils
#include <cstring>
#include <string>
#include <algorithm>

// data structures
#include <vector>
#include <unordered_map>
#include <stack>

// custom utils
#include "__fstatutils__.h"
// #include "__utils__.h" // [keep commented] cuz already included in __fstatutils__


// namespace
using namespace std;

// defines
#define HOME_DIR "/home/jaffrey"

#define umap_s_s unordered_map<string,string>
#define ushort unsigned short

#define NAME_WIDTH 0.275
#define CSIZE_WIDTH 0.15
#define UOWN_WIDTH 0.1
#define GOWN_WIDTH 0.1
#define PERM_WIDTH 0.12

#define MIN_ROW_LISTING 3
#define SPACE_BELOW_LISTING 8

#define NORMAL_MODE true
#define COMMAND_MODE false

#define ALL_PERM 0777
#define OTHER_READ_PERM 6
#define MY_READ_PERM 0
#define OTHER_WRITE_PERM 7
#define MY_WRITE_PERM 1

#define UP_KEY 'A'
#define DOWN_KEY 'B'
#define LEFT_KEY 'D'
#define RIGHT_KEY 'C'
#define ENTER_KEY '\n'
#define BACKSPACE_KEY 127
// #define LMOD_WIDTH 0.1

// global vars
struct winsize w;
string home_dir = getenv("HOME");


// debug op file open
string dbg_op_file = "debug_out.txt";
ofstream fout(dbg_op_file);



// --------------------------------------------------------------------------------------------------


bool check_permission_granted(string content_path, char perm_flag){
    struct stat content_stats;
    lstat(content_path.c_str(), &content_stats);
    string user_own = username_from_id(content_stats.st_uid);
    string perm = get_permissions(content_stats.st_mode);
    // fout << "debug:: uown: " << user_own << endl;
    // fout << "debug:: uname: " << getenv("USERNAME") << endl;
    // fout << "debug:: perm: " << perm << endl;
    bool permission_granted;
    if(perm_flag=='r')
        permission_granted = (perm[OTHER_READ_PERM]==perm_flag ||
                                (getenv("USERNAME")==user_own && perm[MY_READ_PERM]==perm_flag));
    else if(perm_flag=='w')
        permission_granted = (perm[OTHER_WRITE_PERM]==perm_flag ||
                                (getenv("USERNAME")==user_own && perm[MY_WRITE_PERM]==perm_flag));
    return permission_granted;
}


// ----------------------================= COMMAND MODE [b] ===============---------------------------


bool copy_file(string src, string dest){
    fout << "debug:: before OPEN" << endl;
    int src_fd = open(src.c_str(), O_RDONLY); // TODO: handle error
    size_t src_size = get_file_size(src);
    fout << "debug:: src_size: " << src_size << endl;

    fout << "debug:: before read_buffer alloc" << endl;
    char* read_buffer = new char [src_size];
    fout << "debug:: before READ" << endl;
    int read_status = read(src_fd, read_buffer, src_size); // TODO: handle error AND split based on RLIMIT
    fout << "debug:: read_status: " << read_status << endl;
    if (read_status==-1) return false;

    string filename = get_contentname_from_path(src);
    string dest_file_path = dest+"/"+filename;
    fout << "debug:: before WRITE" << endl;
    int dest_fd = open(dest_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, ALL_PERM); // TODO: handle error and permissions
    int write_status = write(dest_fd, read_buffer, src_size); // TODO: handle error
    fout << "debug:: write_status: " << write_status << endl;
    if(write_status==-1) return false;

    fout << "debug:: before CLOSE" << endl;
    close(src_fd); // TODO: handle error
    close(dest_fd); // TODO: handle error
    delete[] read_buffer;

    return true;
}


bool create_file(string filename, string dest){
    string dest_file_path = dest+"/"+filename;
    // fout << "debug:: before WRITE" << endl;
    int dest_fd = open(dest_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, ALL_PERM); // TODO: handle error and permissions
    int write_status = write(dest_fd, "", 0); // TODO: handle error
    // fout << "debug:: write_status: " << write_status << endl;
    if(write_status==-1) return false;

    // fout << "debug:: before CLOSE" << endl;
    close(dest_fd); // TODO: handle error

    return true;
}

bool search_content(string path, string search_term){
    DIR* dir = opendir(path.c_str());

    struct dirent* dir_content;
    struct stat content_stats;
    //initial dir content read
    dir_content = readdir(dir);
    while(dir_content!=NULL){
        string content_name = dir_content->d_name;
        if(content_name == search_term){
            fout << "debug:: found_in : " << path << endl;
            closedir(dir);
            return true;
        }
        string my_path = (path=="/")?(string(path)+content_name):(string(path)+"/"+content_name);
        lstat(my_path.c_str(), &content_stats);
        // fout << "debug:: path : " << path << endl;
        bool permission_granted = check_permission_granted(my_path, 'r');
        string content_type = get_content_type(&content_stats);
        if(content_type == "dir" && content_name!="." && content_name!=".." && permission_granted){
            if(search_content(my_path, search_term) ){
                closedir(dir);
                return true;
            }
        }
        //read next content in dir
        dir_content = readdir(dir);
    }
    closedir(dir);
    return false;
}


bool rename_file(string orig_path, string new_path){
    if(rename(orig_path.c_str(), new_path.c_str()) != -1) return true;
    else return false;
}

bool delete_content(string path){
    if(remove(path.c_str()) != -1) return true;
    else return false;
}

bool create_dir(string path){
    if(mkdir(path.c_str(), ALL_PERM) != -1) return true;
    else return false;
}


bool delete_dir(string path){
    DIR* dir = opendir(path.c_str());
    if(dir == NULL) return false;
    struct dirent* dir_content;
    struct stat content_stats;

    fout << "debug:: path_contents_to_delete: " << path << endl;

    //initial dir content read
    dir_content = readdir(dir);
    while(dir_content!=NULL){
        string content_name = dir_content->d_name;
        string my_path = (path=="/")?(string(path)+content_name):(string(path)+"/"+content_name);

        lstat(my_path.c_str(), &content_stats);
        fout << "debug:: my_path : " << my_path << endl;
        bool permission_granted = check_permission_granted(my_path, 'w');
        string content_type = get_content_type(&content_stats);
        fout << "debug:: to delete[d]: " << my_path << "permission_granted: " << permission_granted << endl;
        if(content_type == "dir" && content_name!="." && content_name!=".." && permission_granted){
            fout << "debug:: to delete[d]: " << my_path << endl;
            if(!delete_dir(my_path)){
                fout << "debug:: couldnt delete[d]: " << my_path << endl;
                closedir(dir);
                return false;
            }
            fout << "debug:: deleted[d]: " << my_path << endl;
        }
        else if(content_type != "dir" && permission_granted){
            fout << "debug:: to delete[f]: " << my_path << endl;
            if(!delete_content(my_path)){
                fout << "debug:: couldnt delete: " << my_path << endl;
                closedir(dir);
                return false;
            }
            fout << "debug:: deleted: " << my_path << endl;
        }
        else if(content_name=="." || content_name==".."){
            // idk lol
        }
        else{
            fout << "debug:: couldnt delete (perm issue ig): " << my_path << endl;
            closedir(dir);
            return false;
        }
        //read next content in dir
        dir_content = readdir(dir);
    }
    closedir(dir);
    if(delete_content(path)){
        fout << "debug:: deleted: " << path << endl;
        return true;
    }
    else
        return false;
}


bool copy_dir(string src, string dest){
    string new_dir_name = get_contentname_from_path(src);
    string dest_path = (dest=="/")?(string(dest)+new_dir_name):(string(dest)+"/"+new_dir_name);
    fout << "debug:: init: dest_path : " << dest_path << endl;
    create_dir(dest_path);

    DIR* dir = opendir(src.c_str());
    struct dirent* dir_content;
    struct stat content_stats;

    //initial dir content read
    dir_content = readdir(dir);
    while(dir_content!=NULL){
        string content_name = dir_content->d_name;
        string src_path = (src=="/")?(string(src)+content_name):(string(src)+"/"+content_name);

        lstat(src_path.c_str(), &content_stats);
        fout << "debug:: src_path : " <<src_path << endl;
        bool permission_granted = check_permission_granted(src_path, 'r');
        string content_type = get_content_type(&content_stats);
        if(content_type == "dir" && content_name!="." && content_name!=".." && permission_granted){
            fout << "debug:: src_path : " <<src_path << endl;
            if(!copy_dir(src_path, dest_path)){
                fout << "debug:: couldnt copy[d]: " << src_path << endl;
                closedir(dir);
                return false;
            }
            fout << "debug:: copied[d]: " << src_path << endl;
        }
        else if(content_type != "dir" && permission_granted){
            if(!copy_file(src_path, dest_path)){
                fout << "debug:: couldnt copy: " << src_path << endl;
                closedir(dir);
                return false;
            }
            fout << "debug:: copied: " << src_path << endl;
        }
        else if(content_name=="." || content_name==".."){
            // idk lol
        }
        else{
            fout << "debug:: couldnt copy(perm issue ig): " << src_path << endl;
            closedir(dir);
            return false;
        }
        //read next content in dir
        dir_content = readdir(dir);
    }
    closedir(dir);
    return true;
}


bool move_dir(string src, string dest){
    return (copy_dir(src, dest) && delete_dir(src));
}


vector<string> parse_command(string command){
    fout << "debug:: ================== parse_command ========================"  << endl;
    fout << "debug:: command: " << command << endl;
    string cmd_token = "";
    vector<string> cmd_tokens;
    for(char& c: command){
        if(c!=' ')
            cmd_token+=c;
        else{
            cmd_tokens.push_back(cmd_token);
            fout << "debug:: cmd_token " << cmd_token << endl;
            cmd_token = "";
        }
    }
    fout << "debug:: cmd_token " << cmd_token << endl;
    cmd_tokens.push_back(cmd_token);
    return cmd_tokens;
}
// ----------------------================= COMMAND MODE [e] ===============---------------------------


void set_content_metadata(umap_s_s& content_map) {
    char* content_path = s_to_carr(content_map["my_path"]);
    struct stat content_stats;
    lstat(content_path, &content_stats);
    content_map["size"] = get_readable_size((content_stats.st_size));
    content_map["uown"] = username_from_id(content_stats.st_uid);
    content_map["gown"] = groupname_from_id(content_stats.st_gid);
    content_map["perm"] = get_permissions(content_stats.st_mode);
    content_map["last_modified"] = trim_newline(ctime(&(content_stats.st_mtime)));
    // fout <<  "debug:: " << content_map["last_modified"] << "abcd" << endl;
    content_map["type"] = get_content_type(&content_stats);
    delete[] content_path;
}

vector<umap_s_s> get_dir_contents(string& path) {
    fout << "debug:: ======================= get_dir_contents ========================" << endl;
    vector<umap_s_s> contents;
    umap_s_s content_map;

    fout << "debug:: path: " << path << endl;

    // create char array for path to abide by ISO type regulations
    char* path_carr =  s_to_carr(path);

    DIR* dir = opendir(path_carr);

    struct dirent* dir_content;
    fout << "debug:: ============ about to init readdir ======================" << endl;
    //initial dir content read
    try{
        dir_content = readdir(dir);
    }
    catch(...){
        fout << "error:: initial read kill" << endl;
    }
    fout << "debug:: ============== init readdir DONE ========================" << endl;
    while(dir_content!=NULL){
        string content_name = dir_content->d_name;
        content_map["name"] = content_name;
        content_map["my_dir_path"] = path;
        content_map["my_path"] = (path=="/")?(string(path)+content_name):(string(path)+"/"+content_name);
        set_content_metadata(content_map);
        contents.push_back(content_map);
        //read next content in dir
        try{
            dir_content = readdir(dir);
        }
        catch(...){
            fout << "error:: intermediate read kill" << endl;
        }
    }
    fout << "debug:: =================== readdir DONE ========================" << endl;

    // sort contents based on content name
    sort(contents.begin(), contents.end(), cmp_content_name);

    delete[] path_carr;

    closedir(dir);

    return contents;
}


void print_dir_contents(
    vector<umap_s_s>& contents
    , bool mode
    , int cursor_pos
    , int start, int end
    , int term_w, int term_h
    , string last_command="NA"
    , string last_command_status="NA"
){
    fout << endl << "debug :: =================  print_dir_contents =================" << endl;
    fout << "debug :: term_w: " << term_w << endl;
    int cols_left;

    // set cout align
    cout.setf(ios::left);

    // clrscr();
    cls();

    for(int content_no=start; content_no<=end; content_no++){
        fout << "debug :: content_no: " << content_no << endl;
        auto& content = contents[content_no];
        fout << "debug :: got content: " << endl;
        fout << "debug :: content_name: "<< content["name"] << endl;
        cols_left = term_w;

        cout.width(NAME_WIDTH*term_w);
        string trimmed_name = (content["name"]).substr(
                            0
                            ,_min(content["name"].length(),(NAME_WIDTH*term_w)-10)
                        );
        fout << "debug :: trimmed_name: " << trimmed_name << endl;
        if(content_no == cursor_pos){
            cout << " ᐉ  " + get_content_icon(content["type"]) + " " + trimmed_name;
            cout << "  ";
        }
        else{
            cout << "     " + get_content_icon(content["type"]) + " " + trimmed_name;
        }
        cols_left -= NAME_WIDTH*term_w;

        cout.unsetf(ios::left);
        cout.setf(ios::right);
        cout.width(CSIZE_WIDTH*term_w);
        cout << content["size"] << "  ";
        cout.unsetf(ios::right);
        cout.setf(ios::left);
        cols_left -= CSIZE_WIDTH*term_w;

        cout.width(UOWN_WIDTH*term_w);
        cout << content["uown"] + " ";
        cols_left -= UOWN_WIDTH*term_w;

        cout.width(GOWN_WIDTH*term_w);
        cout << content["gown"] + " ";
        cols_left -= GOWN_WIDTH*term_w;

        cout.width(PERM_WIDTH*term_w);
        cout << content["perm"] + " ";
        cols_left -= PERM_WIDTH*term_w;

        int lmod_trim_after = _min(content["last_modified"].length(), cols_left);
        // fout << "debug :: lmod_trim_after: " << lmod_trim_after << endl;
        cout << (content["last_modified"]).substr(0,lmod_trim_after);
        cout << endl;
    }
    fout << "debug :: ====== out of the print contents loop======== " << endl;
    if(mode==NORMAL_MODE){
        for(int i=(end-start)+1; i<term_h-2; i++) cout << endl;
        cout << " 🅽 🅾 🆁 🅼 🅰 🅻 - 🅼 🅾 🅳 🅴" << endl;
        cout << " 📂: "<< contents[0]["my_dir_path"];
    }
    else{
        for(int i=(end-start)+1; i<term_h-6; i++) cout << endl;
        cout << " 🅲 🅾 🅼 🅼 🅰 🅽 🅳 - 🅼 🅾 🅳 🅴" << endl;
        cout << " 📂: "<< contents[0]["my_dir_path"] << endl;
        string last_command_trimmed = last_command;
        if(last_command!="NA" && (int)last_command.length()-10 > term_w){
            last_command_trimmed = last_command.substr(0, term_w-10) + "...";
        }
        cout << " ⤺❯❯❯ 🔡 : " << last_command_trimmed << endl;
        cout << " ⤺❯❯❯ ℹ️  : " << last_command_status << endl;
        cout << " ❯❯❯ ";
    }
}



void handle_terminal_resizing(
    vector<umap_s_s>& contents
    , bool& mode
    , int& cursor_pos
    , int& start, int&end
    , int& prev_term_w, int& prev_term_h
    , int& curr_term_w, int& curr_term_h
    , string& command, string& command_status
){
 // recalc terminal dims
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        curr_term_h = w.ws_row;
        curr_term_w = w.ws_col;

        //if width changed reprint
        if(curr_term_w != prev_term_w){
            fout << "debug:: terminal width changed!" << endl;
            print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h
                , command, command_status);
            prev_term_w = curr_term_w;
        }
        //if height changed reprint
        if(curr_term_h != prev_term_h){
            int diff = curr_term_h - prev_term_h;
            fout << endl << "debug:: -----------------------------------" << endl;
            fout << "debug:: terminal height changed!" << endl;
            int rows_available = curr_term_h-SPACE_BELOW_LISTING;
            int new_start = start;
            int new_end = end;
            if(diff<0){
                if(rows_available < contents.size()){
                    new_end = _max(cursor_pos, end-abs(diff));
                    if(cursor_pos>new_end)
                        new_start = _min(cursor_pos, start+abs(diff));
                }
            }
            else{
                if(end-start+1 < contents.size()){
                    new_end = _min(end+abs(diff), contents.size()-1);
                    if(cursor_pos>new_end)
                        new_start = _max(0,start-abs(diff));
                }
            }
            start = new_start;
            end = new_end;
            fout << "debug:: curr_term_h: " << curr_term_h << "; rows_available: " << rows_available << "; diff: " << diff << endl;
            fout << "debug:: cursor_pos: " << cursor_pos << "; start: " << start << "; end: " << end << endl;
            print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h
                , command, command_status);
            prev_term_h = curr_term_h;
            fout << "debug:: RE:printed due to terminal height change!" << endl;
        }
}


int main() {
    vector<umap_s_s> contents;
    string current_dir = home_dir;
    int cursor_pos = 0;
    int start = 0;
    bool mode = NORMAL_MODE;

    stack<string> undo_stack;
    stack<string> redo_stack;

    string command = "NA";
    string command_status = "NA";

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int prev_term_h = w.ws_row;
    int prev_term_w = w.ws_col;
    int curr_term_h = prev_term_h;
    int curr_term_w = prev_term_w;
    contents = get_dir_contents(current_dir);
    int end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
    fout << "debug:: end: " << end << endl;
    print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);

    while(true){
        // check keypress
        if(kbhit()){
            char key = getch();
            if(key == UP_KEY){
                if(cursor_pos!=start){
                    cursor_pos--;
                    fout << "debug:: cursor_pointing to: "<< contents[cursor_pos]["my_path"] << endl;
                    fout << "debug:: ▲ cursor_pos: "<< cursor_pos << "; start: " << start << "; end: " << end << endl;
                    print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                }
                else{
                    if(start > 0){
                        start--;
                        end--;
                        cursor_pos--;
                        fout << "debug:: cursor_pointing to: "<< contents[cursor_pos]["my_path"] << endl;
                        fout << "debug:: ▲ cursor_pos: "<< cursor_pos << "; start: " << start << "; end: " << end << endl;
                        print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                    }
                }
            }
            else if(key == DOWN_KEY){
                if(cursor_pos!=end){
                    cursor_pos++;
                    fout << "debug:: cursor_pointing to: "<< contents[cursor_pos]["my_path"] << endl;
                    fout << "debug:: ▼ cursor_pos: "<< cursor_pos << "; start: " << start << "; end: " << end << endl;
                    print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                }
                else{
                    if(end < contents.size()-1){
                        start++;
                        end++;
                        cursor_pos++;
                        fout << "debug:: cursor_pointing to: "<< contents[cursor_pos]["my_path"] << endl;
                        fout << "debug:: ▼ cursor_pos: "<< cursor_pos << "; start: " << start << "; end: " << end << endl;
                        print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                    }
                }
            }
            else if(key == ENTER_KEY){
                fout << endl << "debug::============================ENTER_PRESS========================" << endl;
                fout << "debug:: name: " << contents[cursor_pos]["name"] << endl;
                fout << "debug:: type: " << contents[cursor_pos]["type"] << endl;
                fout << "debug:: path: " << contents[cursor_pos]["my_path"] << endl;
                if(contents[cursor_pos]["type"] == "dir"
                    && contents[cursor_pos]["name"]!="."
                    && if_permission_granted(contents[cursor_pos])
                    && !(contents[cursor_pos]["name"]==".." && current_dir=="/")
                ){
                    undo_stack.push(current_dir);
                    fout << "debug:: ```" << undo_stack.top() << "``` pushed to undo_stack" << endl;
                    if (contents[cursor_pos]["name"]==".."){
                        current_dir = get_prev_directory_from_path(current_dir);
                    }
                    else{
                        current_dir = contents[cursor_pos]["my_path"];
                    }
                    nuke_stack_contents(redo_stack);
                    fout << "debug:: curr_dir: "<< current_dir << endl;
                    contents = get_dir_contents(current_dir);
                    fout << "debug:: contents_size: " << contents.size() << endl;
                    start = 0;
                    end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                    cursor_pos = 0;
                    print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                }
                else if(contents[cursor_pos]["type"] == "file"){
                    fout << "debug:: FILEOPEN: " << contents[cursor_pos]["my_path"] << endl;
                    if(!fork()) execlp("xdg-open", "xdg-open", contents[cursor_pos]["my_path"].c_str(), NULL);
                    print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                }
            }
            else if(key == BACKSPACE_KEY  && current_dir!="/"){
                undo_stack.push(current_dir);
                nuke_stack_contents(redo_stack);
                current_dir = get_prev_directory_from_path(current_dir);
                contents = get_dir_contents(current_dir);
                start = 0;
                end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                cursor_pos = 0;
                print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
            }
            else if(key == 'h' && current_dir!=home_dir){
                undo_stack.push(current_dir);
                nuke_stack_contents(redo_stack);
                current_dir = home_dir;
                contents = get_dir_contents(current_dir);
                start = 0;
                end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                cursor_pos = 0;
                print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
            }
            else if(key == LEFT_KEY){
                if(!undo_stack.empty()){
                    fout << endl << "debug:: =====================LEFT_KEY==================================" << endl;
                    fout << "debug:: [bfor] undo: " << ((!undo_stack.empty())?(undo_stack.top()) : "-x-");
                    fout << " redo: " << ((!redo_stack.empty())?(redo_stack.top()) : "-x-") << endl;
                    redo_stack.push(current_dir);
                    fout << "debug:: ```" << redo_stack.top() << "``` pushed to redo_stack" << endl;
                    current_dir = undo_stack.top();
                    undo_stack.pop();
                    fout << "debug:: [after] undo: " << ((!undo_stack.empty())?(undo_stack.top()) : "-x-");
                    fout << " redo: " << ((!redo_stack.empty())?(redo_stack.top()) : "-x-") << endl;
                    contents = get_dir_contents(current_dir);
                    start = 0;
                    end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                    cursor_pos = 0;
                    print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                }
            }
            else if(key == RIGHT_KEY){
                if(!redo_stack.empty()){
                    fout << endl << "debug:: =====================RIGHT_KEY==================================" << endl;
                    fout << "debug:: [bfor] undo: " << ((!undo_stack.empty())?(undo_stack.top()) : "-x-");
                    fout << " redo: " << ((!redo_stack.empty())?(redo_stack.top()) : "-x-") << endl;
                    undo_stack.push(current_dir);
                    fout << "debug:: ```" << undo_stack.top() << "``` pushed to undo_stack" << endl;
                    current_dir = redo_stack.top();
                    redo_stack.pop();
                    fout << "debug:: [after] undo: " << ((!undo_stack.empty())?(undo_stack.top()) : "-x-");
                    fout << " redo: " << ((!redo_stack.empty())?(redo_stack.top()) : "-x-") << endl;
                    contents = get_dir_contents(current_dir);
                    start = 0;
                    end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                    cursor_pos = 0;
                    print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h);
                }
            }
            else if(key == ':'){
                mode = COMMAND_MODE;
                print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h
                    , command, command_status);
                while(true){
                    getline(cin, command);
                    vector<string> cmd_tokens = parse_command(command);
                    int cmd_tokens_size = cmd_tokens.size();
                    if(cmd_tokens[0] == "quit"){
                        cls();
                        return 0;
                    }
                    else if(cmd_tokens[0] == "copy"){
                        bool res=true;
                        bool status;
                        string copy_to = path_ka_jugaad_kar(current_dir, cmd_tokens[cmd_tokens_size-1]);
                        for(int i=1; i<cmd_tokens_size-1; i++){
                            string to_copy = cmd_tokens[i];
                            to_copy = path_ka_jugaad_kar(current_dir, to_copy);
                            fout << "debug:: to_copy: " << to_copy  << endl;
                            string content_type = get_content_type_from_path(to_copy);
                            fout << "debug:: content_type: " << content_type  << endl;
                            if(content_type=="file"){
                                status = copy_file(to_copy, copy_to);
                                fout << "debug:: to_copy: " << to_copy  << endl;
                                fout << "debug:: copy_to: " << copy_to  << "; status: " << status << endl;
                            }
                            else if(content_type=="dir"){
                                status = copy_dir(to_copy, copy_to);
                                fout << "debug:: to_copy[d]: " << to_copy  << endl;
                                fout << "debug:: copy_to: " << copy_to  << "; status: " << status << endl;
                            }
                            else{
                                status = false;
                                fout << "debug:: (fail) to_copy: " << to_copy  << endl;
                                fout << "debug:: (fail) copy_to: " << copy_to  << "; status: " << status << endl;
                            }
                            res = res && status;
                        }
                        if(res) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "move"){
                        bool res=true;
                        bool status;
                        string move_to = path_ka_jugaad_kar(current_dir, cmd_tokens[cmd_tokens_size-1]);
                        for(int i=1; i<cmd_tokens_size-1; i++){
                            string to_move = cmd_tokens[i];
                            to_move = path_ka_jugaad_kar(current_dir, to_move);
                            string content_type = get_content_type_from_path(to_move);
                            if(content_type=="file"){
                                string filename = get_contentname_from_path(to_move);
                                string file_new_path = move_to + ((move_to!="/")?"/":"") + filename;
                                status = rename_file(to_move, file_new_path);
                                fout << "debug:: to_move: " << to_move  << endl;
                                fout << "debug:: move_to: " << file_new_path  << "; status: " << status << endl;
                            }
                            else if(content_type=="dir"){
                                status = move_dir(to_move, move_to);
                                fout << "debug:: to_move[d]: " << to_move  << endl;
                                fout << "debug:: move_to: " << move_to  << "; status: " << status << endl;
                            }
                            res = res && status;
                        }
                        if(res) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "rename"){
                        string to_rename = path_ka_jugaad_kar(current_dir, cmd_tokens[1]);
                        string rename_to = path_ka_jugaad_kar(current_dir, cmd_tokens[2]);
                        bool status = rename_file(to_rename, rename_to);
                        fout << "debug:: to_rename: " << to_rename  << endl;
                        fout << "debug:: rename_to: " << rename_to  << "; status: " << status << endl;
                        if(status) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "create_file"){
                        string filename = cmd_tokens[1];
                        string dest = path_ka_jugaad_kar(current_dir, cmd_tokens[2]);
                        bool status =  create_file(filename, dest);
                        fout << "debug:: create filename: " << filename  << endl;
                        fout << "debug:: dest: " << dest  << "; status: " << status << endl;
                        if(status) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "create_dir"){
                        string dirname = cmd_tokens[1];
                        string path = path_ka_jugaad_kar(current_dir, cmd_tokens[2]);
                        path = path + ((path=="/")?"":"/") + dirname;
                        bool status =  create_dir(path);
                        fout << "debug:: create dirname: " << dirname  << endl;
                        fout << "debug:: create dir: " << path  << "; status: " << status << endl;
                        if(status) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "delete_file"){
                        string path = path_ka_jugaad_kar(current_dir, cmd_tokens[1]);
                        bool status =  delete_content(path);
                        fout << "debug:: delete file: " << path  << "; status: " << status << endl;
                        if(status) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "delete_dir"){
                        string path = path_ka_jugaad_kar(current_dir, cmd_tokens[1]);
                        bool status =  delete_dir(path);
                        fout << "debug:: delete dir: " << path  << "; status: " << status << endl;
                        if(status) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "goto"){
                        bool status = true;
                        string path = path_ka_jugaad_kar(current_dir, cmd_tokens[1]);
                        fout << "debug:: goto:" << path << endl;
                        int fd = open(path.c_str(), O_RDONLY);
                        if(fd==-1){
                            status = false;
                        }
                        else{
                            close(fd);
                            contents = get_dir_contents(path);
                            fout << "debug:: contentslen:" << contents.size() << endl;
                            current_dir = path;
                            if(path!=current_dir){
                                undo_stack.push(current_dir);
                                nuke_stack_contents(redo_stack);
                            }
                        }
                        if(status) command_status = "SUCCESS!";
                        else command_status = "FAILED!";

                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                            , start, end
                                            , curr_term_w, curr_term_h
                                            , command, command_status
                                        );
                    }
                    else if(cmd_tokens[0] == "search"){
                        string path = current_dir;
                        string search_term = cmd_tokens[1];
                        bool status =  search_content(path, search_term);
                        fout << "debug:: search: "  << search_term << endl;
                        fout << "debug:: path: " << path  << "; status: " << status << endl;
                        if(status) command_status = "SUCCESS!";
                        else command_status = "FAILED!";


                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                    else if(cmd_tokens[0] == "switch"){ // TODO:: mode switch should be invoked on ESC key
                        mode = NORMAL_MODE;
                        command_status="SUCCESS!";
                        print_dir_contents(contents, mode, cursor_pos, start, end, curr_term_w, curr_term_h,command,command_status);
                        break;
                    }
                    else{
                        start = 0;
                        end = _min(curr_term_h-SPACE_BELOW_LISTING, contents.size()-1);
                        command_status = "UNKNOWN command";
                        print_dir_contents(contents, mode, cursor_pos
                                        , start, end
                                        , curr_term_w, curr_term_h
                                        , command, command_status
                                    );
                    }
                }

            }
            else if(key == 'q'){
                cls();
                break;
            }
        }

        handle_terminal_resizing(
            contents
            , mode
            , cursor_pos
            , start, end
            , prev_term_w, prev_term_h
            , curr_term_w, curr_term_h
            , command, command_status
        );
    }
    fout.close();
}