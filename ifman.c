#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<dirent.h>
#include<curl/curl.h>
#include<getopt.h>
#include<sys/stat.h>

#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))

/*
 * HELP MODULE
 */
int help() {
    printf("Interactive Fiction Manager Help\n"
            "\t--help\n\t\tdisplay this help and exit\n"
            "\t--version\n\t\tdisplay version of this program\n"
            "\tinit\n\t\tinitializes the directory structure and index file\n"
            "\tmanage\n\t\tmanages the games on your system. Use one of the following flags:\n"
            "\t\t-S, --install <game>\n\t\t\tdownloads <game> from if-archive.\n"
            "\t\t-D, --remove <game>\n\t\t\tremoves <game> from your system.\n"
            "\t\t-l, --list\n\t\t\tlists all games on if-archive.\n"
            "\tplay <game>\n\t\topens <game> with frotz.\n");
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
            fprintf(fp2, "%s", link);
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
void list(char *arg) {
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
        if (arg) {
            size_t len = strlen(arg);
            if (strncmp(arg, start, len) == 0) {
                printf("%s", line);
            }
        } else {
            printf("%s", line);
        }
    }

    fclose(fp);
}

void installed() {
    char *home_directory = getenv("HOME");

    char install_file_path[1048];
    snprintf(install_file_path, sizeof(install_file_path), "%s/.ifman/games", home_directory);
    
    struct dirent *files;
    DIR *dir = opendir(install_file_path);

    if (dir == NULL) {
        printf("Install directory cannot be opened!\n");
        return ;
    }
    while ((files = readdir(dir)) != NULL) {
        if (files->d_name[0] != '.') {
            printf("%s\n", files->d_name);
        }
    }
    closedir(dir);
}

void install_game(char *line) {
    CURL *curl;
    FILE *fp;
    int result;

    char link[50];

    strcpy(link, line);
    size_t len = strlen(link);
    if (len > 0 && link[len-1] == '\n') {
        link[len-1] = '\0';
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
        printf("Downloaded %s successfully!\n", link);
    } else {
        printf("Error: %s\n", curl_easy_strerror(result));
    }
    fclose(fp);
    curl_easy_cleanup(curl);
}

void install(char *arg) {
    install_game(arg);
}

void uninstall_game(char *line) {
    char link[50];
    strcpy(link, line);
    size_t len = strlen(link);
    if (len > 0 && link[len-1] == '\n') {
        link[len-1] = '\0';
    }

    char *home_directory = getenv("HOME");
    char file_path[1048];
    snprintf(file_path, sizeof(file_path), "%s/.ifman/games/%s", home_directory, link);
    remove(file_path);
    printf("Removed game %s\n", link);
}

void uninstall(char *arg) {
    uninstall_game(arg);
}

int manage(int argc, char **argv) {
    int opt;
    static struct option long_options[] = {
        {"list", optional_argument, NULL, 'l'},
        {"installed", no_argument, 0, 'L'},
        {"install", required_argument, 0, 'S'},
        {"remove", required_argument, 0, 'D'},
        {0, 0, 0, 0}
    };
    opt = getopt_long(argc, argv, "l::LS:D:", long_options, 0);
    switch (opt) {
        case 'l':
            OPTIONAL_ARGUMENT_IS_PRESENT;
            list(optarg);
            break;
        case 'L':
            installed();
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

    char *home_directory = getenv("HOME");

    char game_path[1048];
    snprintf(game_path, sizeof(game_path), "%s/.ifman/games/%s", home_directory, game);

    char command[256];
    snprintf(command, sizeof(command), "frotz %s", game_path);
    printf("%s\n", command);

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
