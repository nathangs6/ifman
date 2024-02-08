#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<curl/curl.h>
#include<getopt.h>
#include<sys/stat.h>

/*
 * HELP MODULE
 */
int help() {
    printf("Interactive Fiction Manager Help\nName\n\tifman - manages interactive fiction files and players.\nDescription\n\t--help display this help and exit\n");
    return 0;
}

/* 
 * VERSION MODULE
 */
int version() {
    printf("Interactive Fiction Manager (0.1)\n");
    return 0;
}

/*
 * INIT MODULE
 */
int create_directory() {
    char *home_directory = getenv("HOME");
    if (home_directory == NULL) {
        printf("Error: HOME environment variable not set.\n");
        return 1;
    }

    char directory_path[1024];
    snprintf(directory_path, sizeof(directory_path), "%s/.ifman", home_directory);

    if (mkdir(directory_path, 0777) == -1) {
        perror("Error creating directory.");
        return 1;
    }

    snprintf(directory_path, sizeof(directory_path), "%s/.ifman/games", home_directory);

    if (mkdir(directory_path, 0777) == -1) {
        perror("Error creating directory.");
        return 1;
    }

    printf("Directory ~/.ifman created succesfully.\n");
    return 0;
}

int get_index() {
    CURL *curl;
    FILE *fp;
    int result;

    char *home_directory = getenv("HOME");
    char file_path[1048];
    snprintf(file_path, sizeof(file_path), "%s/.ifman/temp_init.txt", home_directory);

    fp = fopen(file_path, "wb");

    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, "https://www.ifarchive.org/if-archive/games/zcode/");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    result = curl_easy_perform(curl);

    if (result == CURLE_OK) {
        printf("Downloaded index successfully!\n");
    } else {
        printf("Error: %s\n", curl_easy_strerror(result));
    }
    fclose(fp);
    curl_easy_cleanup(curl);
    return 0;
}

typedef struct {
    char name[100];
    char link[100];
} GameInfo;

void construct_index() {
    FILE *fp1;
    FILE *fp2;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char name[50];
    char link[50];


    char *home_directory = getenv("HOME");
    char temp_file_path[1048];
    snprintf(temp_file_path, sizeof(temp_file_path), "%s/.ifman/temp_init.txt", home_directory);
    fp1 = fopen(temp_file_path, "r");

    char index_file_path[1048];
    snprintf(index_file_path, sizeof(index_file_path), "%s/.ifman/index.csv", home_directory);
    fp2 = fopen(index_file_path, "w");

    while ((read = getline(&line, &len, fp1)) != -1) {
        if (line[0] == '#') {
            char *start = line;
            while (*start == '#' || *start == ' ') {
                start++;
            }
            strcpy(link, start);

            char *dot = strchr(start, '.');
            if (dot != NULL) {
                int name_length = dot - start;
                strncpy(name, start, name_length);
                name[name_length] = '\0';
                fprintf(fp2, "%s,%s", name, link);
            }
        }
    }

    fclose(fp1);
    fclose(fp2);
    remove(temp_file_path);
}

int init(int argc, char **argv) {
    if (create_directory() != 0) {
        return 1;
    }
    get_index();
    construct_index();
    printf("Index created successfully!\n");
    return 0;
}

/*
 * MANAGE MODULE
 */
void list() {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;


    char *home_directory = getenv("HOME");

    char index_file_path[1048];
    snprintf(index_file_path, sizeof(index_file_path), "%s/.ifman/index.csv", home_directory);
    fp = fopen(index_file_path, "r");

    while ((read = getline(&line, &len, fp)) != -1) {
        char *start = line;
        char *comma = strchr(start, ',');
        int length = comma - start;
        printf("%.*s\n", length, line);
    }

    fclose(fp);
}

void install_game(char *line) {
    CURL *curl;
    FILE *fp;
    int result;

    char name[50];
    char link[50];
    char *comma = strchr(line, ',');
    if (comma != NULL) {
        int length = comma - line;
        strncpy(name, line, length);
        name[length] = '\0';
        strcpy(link, comma + 1);
        size_t len = strlen(link);
        if (len > 0 && link[len-1] == '\n') {
            link[len-1] = '\0';
        }
    } else {
        printf("Name and link couldn't be split to install the game!\n");
        return ;
    }

    char *home_directory = getenv("HOME");
    char file_path[1048];
    snprintf(file_path, sizeof(file_path), "%s/.ifman/games/%s", home_directory, link);

    fp = fopen(file_path, "wb");

    curl = curl_easy_init();

    char url[100];
    strcpy(url, "https://www.ifarchive.org/if-archive/games/zcode/");
    strcat(url, link);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    result = curl_easy_perform(curl);

    if (result == CURLE_OK) {
        printf("Downloaded %s successfully!\n", name);
    } else {
        printf("Error: %s\n", curl_easy_strerror(result));
    }
    fclose(fp);
    curl_easy_cleanup(curl);
}

void install(char *arg) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;


    char *home_directory = getenv("HOME");

    char index_file_path[1048];
    snprintf(index_file_path, sizeof(index_file_path), "%s/.ifman/index.csv", home_directory);
    fp = fopen(index_file_path, "r");

    while ((read = getline(&line, &len, fp)) != -1) {
        char *start = line;
        char *comma = strchr(start, ',');
        int length = comma - start;
        
        if (strncmp(line, arg, length) == 0) {
            install_game(line);
            return;
        }
    }
    printf("File not found!\n");
    
    fclose(fp);
}

void uninstall_game(char *line) {
    CURL *curl;
    FILE *fp;
    int result;

    char name[50];
    char link[50];
    char *comma = strchr(line, ',');
    if (comma != NULL) {
        int length = comma - line;
        strncpy(name, line, length);
        name[length] = '\0';
        strcpy(link, comma + 1);
        size_t len = strlen(link);
        if (len > 0 && link[len-1] == '\n') {
            link[len-1] = '\0';
        }
    } else {
        printf("Name and link couldn't be split to install the game!\n");
        return ;
    }

    char *home_directory = getenv("HOME");
    char file_path[1048];
    snprintf(file_path, sizeof(file_path), "%s/.ifman/games/%s", home_directory, link);
    remove(file_path);
    printf("Removed game %s\n", name);
}

void uninstall(char *arg) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;


    char *home_directory = getenv("HOME");

    char index_file_path[1048];
    snprintf(index_file_path, sizeof(index_file_path), "%s/.ifman/index.csv", home_directory);
    fp = fopen(index_file_path, "r");

    while ((read = getline(&line, &len, fp)) != -1) {
        char *start = line;
        char *comma = strchr(start, ',');
        int length = comma - start;
        
        if (strncmp(line, arg, length) == 0) {
            uninstall_game(line);
            return;
        }
    }
    printf("File not found!\n");
    
    fclose(fp);
}

int manage(int argc, char **argv) {
    int opt;
    static struct option long_options[] = {
        {"list", no_argument, 0, 'l'},
        {"install", required_argument, 0, 'S'},
        {"remove", required_argument, 0, 'D'},
        {0, 0, 0, 0}
    };
    opt = getopt_long(argc, argv, "hvfS:D:", long_options, 0);
    switch (opt) {
        case 'l':
            list();
            break;
        case 'S':
            install(optarg);
            break;
        case 'D':
            uninstall(optarg);
            break;
        default:
            printf("No valid arguments provided. Try using 'ifman help' if you need help.\n");
    }
    return 0;
}

/*
 * PLAY MODULE
 */
int play(int argc, char **argv) {
    if (argc < 3) {
        printf("Please provide a game to play!\n");
        return 1;
    }
    char game[64];
    strcpy(game, argv[2]);
    int len = strlen(game);
    if (len > 0 && game[len-1] == '\n') {
        game[len-1] = '\0';
    }
    char command[256];
    snprintf(command, sizeof(command), "frotz %s", argv[2]);

    int result = system(command);

    if (result != 0) {
        printf("An error occurred while running the game :(\n");
    }
    return result;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please provide arguments. Try using 'ifman help' if you need help.\n");
        return 0;
    }
    char *command = argv[1];
    if (strcmp(command, "help") == 0) {
        return help();
    } else if (strcmp(command, "version") == 0) {
        return version();
    } else if (strcmp(command, "init") == 0) {
        return init(argc, argv);
    } else if (strcmp(command, "manage") == 0) {
        return manage(argc, argv);
    } else if (strcmp(command, "play") == 0) {
        return play(argc, argv);
    } else {
        int opt;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"version", no_argument, 0, 'v'},
            {0, 0, 0, 0}  
        };
        opt = getopt_long(argc, argv, "hvf:", long_options, 0);
        switch (opt) {
            case 'h':
                help();
                break;
            case 'v':
                version();
                break;
            default:
                printf("No valid arguments provided. Try using 'ifman help' if you need help.\n");
                return 0;
        }
        return 0;
    }
}
