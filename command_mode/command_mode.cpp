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

// namespace
using namespace std;


#define ALL_PERM 0777
#define OTHER_READ_PERM 6
#define MY_READ_PERM 0
#define OTHER_WRITE_PERM 7
#define MY_WRITE_PERM 1


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


string get_contentname_from_path(string path){
    int file_dir_length =  get_prev_directory_from_path(path).length();
    return path.substr(file_dir_length);
}


size_t get_file_size(string src){
    struct stat file_stats;
    lstat(src.c_str(), &file_stats);
    return file_stats.st_size;
}


string get_content_type(struct stat *content_stats){
    string file_type;
    switch (content_stats->st_mode & S_IFMT){
    case S_IFREG:
        file_type = "file";
        break;
    case S_IFLNK:
        file_type = "slink";
        break;
    case S_IFDIR:
        file_type = "dir";
        break;
    case S_IFCHR:
        file_type = "char_device";
        break;
    case S_IFBLK:
        file_type = "block_device";
        break;
    case S_IFIFO:
        file_type = "pipe";
        break;
    case S_IFSOCK:
        file_type = "socket";
        break;
    default:
        file_type = "unknown";
        break;
    }
    return file_type;
}



string get_permissions(mode_t perm) {
    string modeval = "---------";
    modeval[0] = (perm & S_IRUSR) ? 'r' : '-';
    modeval[1] = (perm & S_IWUSR) ? 'w' : '-';
    modeval[2] = (perm & S_IXUSR) ? 'x' : '-';
    modeval[3] = (perm & S_IRGRP) ? 'r' : '-';
    modeval[4] = (perm & S_IWGRP) ? 'w' : '-';
    modeval[5] = (perm & S_IXGRP) ? 'x' : '-';
    modeval[6] = (perm & S_IROTH) ? 'r' : '-';
    modeval[7] = (perm & S_IWOTH) ? 'w' : '-';
    modeval[8] = (perm & S_IXOTH) ? 'x' : '-';
    return modeval;
}

char* username_from_id(uid_t uid) {
    struct passwd *pwd;
    pwd = getpwuid(uid);
    return (pwd == NULL) ? NULL : pwd->pw_name;
}

bool check_permission_granted(string content_path, char perm_flag){
    struct stat content_stats;
    lstat(content_path.c_str(), &content_stats);
    string user_own = username_from_id(content_stats.st_uid);
    string perm = get_permissions(content_stats.st_mode);
    cout << "debug:: uown: " << user_own << endl;
    cout << "debug:: uname: " << getenv("USERNAME") << endl;
    cout << "debug:: perm: " << perm << endl;
    bool permission_granted;
    if(perm_flag=='r')
        permission_granted = (perm[OTHER_READ_PERM]==perm_flag ||
                                (getenv("USERNAME")==user_own && perm[MY_READ_PERM]==perm_flag));
    else if(perm_flag=='w')
        permission_granted = (perm[OTHER_WRITE_PERM]==perm_flag ||
                                (getenv("USERNAME")==user_own && perm[MY_WRITE_PERM]==perm_flag));
    return permission_granted;
}



bool copy_file(string src, string dest){
    cout << "debug:: before OPEN" << endl;
    int src_fd = open(src.c_str(), O_RDONLY); // TODO: handle error
    size_t src_size = get_file_size(src);
    cout << "debug:: src_size: " << src_size << endl;

    cout << "debug:: before read_buffer alloc" << endl;
    char* read_buffer = new char [src_size];
    cout << "debug:: before READ" << endl;
    int read_status = read(src_fd, read_buffer, src_size); // TODO: handle error AND split based on RLIMIT
    cout << "debug:: read_status: " << read_status << endl;
    if (read_status==-1) return false;

    string filename = get_contentname_from_path(src);
    string dest_file_path = dest+"/"+filename;
    cout << "debug:: before WRITE" << endl;
    int dest_fd = open(dest_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, ALL_PERM); // TODO: handle error and permissions
    int write_status = write(dest_fd, read_buffer, src_size); // TODO: handle error
    cout << "debug:: write_status: " << write_status << endl;
    if(write_status==-1) return false;

    cout << "debug:: before CLOSE" << endl;
    close(src_fd); // TODO: handle error
    close(dest_fd); // TODO: handle error
    delete[] read_buffer;

    return true;
}



bool create_file(string filename, string dest){
    string dest_file_path = dest+"/"+filename;
    // cout << "debug:: before WRITE" << endl;
    int dest_fd = open(dest_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, ALL_PERM); // TODO: handle error and permissions
    int write_status = write(dest_fd, "", 0); // TODO: handle error
    // cout << "debug:: write_status: " << write_status << endl;
    if(write_status==-1) return false;

    // cout << "debug:: before CLOSE" << endl;
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
            cout << "debug:: found_in : " << path << endl;
            closedir(dir);
            return true;
        }
        string my_path = (path=="/")?(string(path)+content_name):(string(path)+"/"+content_name);
        lstat(my_path.c_str(), &content_stats);
        // cout << "debug:: path : " << path << endl;
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
    struct dirent* dir_content;
    struct stat content_stats;

    cout << "debug:: path_contents_to_delete: " << path << endl;

    //initial dir content read
    dir_content = readdir(dir);
    while(dir_content!=NULL){
        string content_name = dir_content->d_name;
        string my_path = (path=="/")?(string(path)+content_name):(string(path)+"/"+content_name);

        lstat(my_path.c_str(), &content_stats);
        cout << "debug:: my_path : " << my_path << endl;
        bool permission_granted = check_permission_granted(my_path, 'w');
        string content_type = get_content_type(&content_stats);
        cout << "debug:: to delete[d]: " << my_path << "permission_granted: " << permission_granted << endl;
        if(content_type == "dir" && content_name!="." && content_name!=".." && permission_granted){
            cout << "debug:: to delete[d]: " << my_path << endl;
            if(!delete_dir(my_path)){
                cout << "debug:: couldnt delete[d]: " << my_path << endl;
                closedir(dir);
                return false;
            }
            cout << "debug:: deleted[d]: " << my_path << endl;
        }
        else if(content_type != "dir" && permission_granted){
            cout << "debug:: to delete[f]: " << my_path << endl;
            if(!delete_content(my_path)){
                cout << "debug:: couldnt delete: " << my_path << endl;
                closedir(dir);
                return false;
            }
            cout << "debug:: deleted: " << my_path << endl;
        }
        else if(content_name=="." || content_name==".."){
            // idk lol
        }
        else{
            cout << "debug:: couldnt delete (perm issue ig): " << my_path << endl;
            closedir(dir);
            return false;
        }
        //read next content in dir
        dir_content = readdir(dir);
    }
    closedir(dir);
    if(delete_content(path)){
        cout << "debug:: deleted: " << path << endl;
        return true;
    }
    else
        return false;
}


bool copy_dir(string src, string dest){
    string new_dir_name = get_contentname_from_path(src);
    string dest_path = (dest=="/")?(string(dest)+new_dir_name):(string(dest)+"/"+new_dir_name);
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
        cout << "debug:: src_path : " <<src_path << endl;
        bool permission_granted = check_permission_granted(src_path, 'r');
        string content_type = get_content_type(&content_stats);
        if(content_type == "dir" && content_name!="." && content_name!=".." && permission_granted){
            cout << "debug:: src_path : " <<src_path << endl;
            if(!copy_dir(src_path, dest_path)){
                cout << "debug:: couldnt copy[d]: " << src_path << endl;
                closedir(dir);
                return false;
            }
            cout << "debug:: copied[d]: " << src_path << endl;
        }
        else if(content_type != "dir" && permission_granted){
            if(!copy_file(src_path, dest_path)){
                cout << "debug:: couldnt copy: " << src_path << endl;
                closedir(dir);
                return false;
            }
            cout << "debug:: copied: " << src_path << endl;
        }
        else if(content_name=="." || content_name==".."){
            // idk lol
        }
        else{
            cout << "debug:: couldnt copy(perm issue ig): " << src_path << endl;
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



int main(){
    // string src = "/home/jaffrey/data/test_src/x.pdf";
    // string dest = "/home/jaffrey/data/test_dest";
    // copy_file(src, dest);

    // string path = "/";
    // string search_term = "random";
    // bool search_res = search_content(path, search_term);
    // cout << "debug:: search_res : " << (search_res?"mila":"nai mila") << endl;

    // string path = "/home/jaffrey/data/test_src/lolx";
    // cout << "debug:: delete_content(path): " << delete_content(path) << endl;

    // string path = "/home/jaffrey/data/test_src";
    // string filename = "lol.txt";
    // cout << "debug:: create_file: " << create_file(filename, path) << endl;

    // string old_path = "/home/jaffrey/data/test_src/lol.txt";
    // string new_path = "/home/jaffrey/data/test_src/lmao.txt";
    // cout << "debug:: rename_content(path): " << rename_content(old_path, new_path) << endl;
    // string old_path = "/home/jaffrey/data/test_src/lol.txt";
    // string new_path = "/home/jaffrey/data/test_src/lmao.txt";
    // cout << "debug:: rename_content(path): " << rename_content(old_path, new_path) << endl;

    // string dir_path = "/home/jaffrey/data/test_src/rekt";
    // cout << "debug:: create_dir(dir_path): " << create_dir(dir_path) << endl;


    // string copy_what = "/home/jaffrey/data/test_src";
    // string copy_to = "/home/jaffrey/data/test_dest";
    // cout << "debug:: =======copy_dir:=========== " << endl << copy_dir(copy_what, copy_to) << endl;

    // string dir_path = "/home/jaffrey/data/test_dest/lol";
    // cout << "debug:: =======delete_dir:========= " << endl << delete_dir(dir_path) << endl;

    // string from_path = "/home/jaffrey/data/test_src/lol";
    // string to_path = "/home/jaffrey/data/test_dest/lol_dest";
    // cout << "debug:: =======move_dir:=========== " << endl << move_dir(from_path, to_path) << endl;

    return 0;
}


