#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>

// custom utils
#include "__utils__.h"

using namespace std;

// defines
#define ALL_PERM 0777
#define MY_READ_PERM 0
#define OTHER_READ_PERM 6
#define MY_WRITE_PERM 1
#define OTHER_WRITE_PERM 7
#define GRP_READ_PERM 3



/* Return name corresponding to 'uid', or NULL on error */
char* username_from_id(uid_t uid) {
    struct passwd *pwd;
    pwd = getpwuid(uid);
    return (pwd == NULL) ? NULL : pwd->pw_name;
}


/* Return UID corresponding to 'name', or -1 on error */
uid_t uid_from_name(const char *name) {
    struct passwd *pwd;
    uid_t u;
    char *endptr;
    if (name == NULL || *name == '\0')
        return -1; /* On NULL or empty string */
    /* return an error */
    u = strtol(name, &endptr, 10);
    if (*endptr == '\0')
        return u; /* As a convenience to caller */
    /* allow a numeric string */
    pwd = getpwnam(name);
    if (pwd == NULL)
        return -1;
    return pwd->pw_uid;
}


/* Return name corresponding to 'gid', or NULL on error */
char* groupname_from_id(gid_t gid) {
    struct group *grp;
    grp = getgrgid(gid);
    return (grp == NULL) ? NULL : grp->gr_name;
}


/* Return GID corresponding to 'name', or -1 on error */
gid_t gid_from_name(const char *name) {
    struct group *grp;
    gid_t g;
    char *endptr;
    if (name == NULL || *name == '\0')
        return -1; /* On NULL or empty string */
    /* return an error */
    g = strtol(name, &endptr, 10);
    if (*endptr == '\0')
        return g; /* As a convenience to caller */
    /* allow a numeric string */
    grp = getgrnam(name);
    if (grp == NULL)
        return -1;
    return grp->gr_gid;
}


size_t get_file_size(string src){
    struct stat file_stats;
    lstat(src.c_str(), &file_stats);
    return file_stats.st_size;
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

string get_content_type_from_path(string path){
    string file_type;
    struct stat content_stats;
    lstat(path.c_str(), &content_stats);
    switch (content_stats.st_mode & S_IFMT){
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



string get_content_type(struct stat *content_stats){
    string file_type;
    switch (content_stats->st_mode & S_IFMT)
    {
    case S_IFREG:
        file_type = "file";
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
    case S_IFLNK:
        file_type = "slink";
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


string get_readable_size(size_t size) {
    static const char *SIZES[] = {"B", "KB", "MB", "GB"};
    int div = 0;
    size_t rem = 0;

    while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES))
    {
        rem = (size % 1024);
        div++;
        size /= 1024;
    }

    double size_d = (float)size + (float)rem / 1024.0;
    string result = convert_to_string(round_off(size_d)) + "" + SIZES[div];
    return result;
}
