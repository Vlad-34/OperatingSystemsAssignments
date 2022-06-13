#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int traverse(char* path, char* name_filter, int execute_filter, int recursive, int* results)
{
    DIR *dir = NULL; // directory manipulation variables
    struct dirent *entry = NULL;
    char* full_path = (char*)malloc(512 * sizeof(char));
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL)
    {
        printf("ERROR\ninvalid directory path\n");
        return 1;
    }

    (*results)++;
    if(*results == 1)
        printf("SUCCESS\n");

    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(full_path, 512, "%s/%s", path, entry->d_name);
            if(stat(full_path, &statbuf) == 0)
                if(recursive == 1 && S_ISDIR(statbuf.st_mode)) // if recursive flag is on and current file is a directory
                    traverse(full_path, name_filter, execute_filter, recursive, results);

            char* s =(char*)calloc(strlen(entry->d_name), sizeof(char));
            if(strstr(entry->d_name, name_filter) != NULL)
                snprintf(s, 512, "%s", strstr(entry->d_name, name_filter));
            if(strlen(name_filter) != 0 && strcmp(s, name_filter) == 0) // if name_filter is active
                printf("%s\n", full_path);
                
            else if(execute_filter == 1 && (statbuf.st_mode & 64) == 64) // if execute_filter is active
                printf("%s\n", full_path);
            else if(strlen(name_filter) == 0 && execute_filter == 0) // if no filter is active
                printf("%s\n", full_path);
            free(s);
        }
    }
    free(full_path);
    closedir(dir);
    return 0;
}

int traverse2(const char* path, int* results)
{
    DIR *dir = NULL; // directory manipulation variables
    struct dirent *entry = NULL;
    char full_path[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL)
    {
        printf("ERROR\ninvalid directory path\n");
        return 1;
    }
    
    (*results)++;
    if(*results == 1)
        printf("SUCCESS\n");

    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0)
        {
            snprintf(full_path, 512, "%s/%s", path, entry->d_name);
            if(stat(full_path, &statbuf) == 0)
                if(S_ISDIR(statbuf.st_mode)) // if recursive flag is on and current file is a directory
                    traverse2(full_path, results);


            if(!S_ISDIR(statbuf.st_mode))
            {
                int fd = -1;
                fd = open(full_path, O_RDONLY);
                if(fd == -1)
                {
                    printf("ERROR\ninvalid file\n");
                    return 1;
                }

                char magic[3];
                int header_size, version = 0, no_of_sections = 0;
                char sect_name[14][9] = {0};
                int sect_type[14] = {0}, sect_offset[14] = {0}, sect_size[14] = {0};
                int valid = 1; // we assume the file is valid

                //printf("%s: %ld\n", full_path, lseek(fd, 0, SEEK_SET));
                if(read(fd, magic, 2) != 2)
                {
                    printf("ERROR\nreading error\n");
                    return 1;
                }
                if(read(fd, &header_size, 2) != 2)
                {
                    printf("ERROR\nreading error\n");
                    return 1;
                }
                if(read(fd, &version, 2) != 2)
                {
                    printf("ERROR\nreading error\n");
                    return 1;
                }
                if(read(fd, &no_of_sections, 1) != 1)
                {
                    printf("ERROR\nreading error\n");
                    return 1;
                }

                if(strcmp(magic, "Gr") != 0)
                {
                    // printf("ERROR\nwrong magic\n");
                    valid = 0;
                    close(fd);
                    closedir(dir);
                    return 1;
                }
                else if(version < 83 || version > 185)
                {
                    // printf("ERROR\nwrong version\n");
                    valid = 0;
                    close(fd);
                    closedir(dir);
                    return 1;
                }
                else if(no_of_sections < 8 || no_of_sections > 14)
                {
                    // printf("ERROR\nwrong sect_nr\n");
                    valid = 0;
                    close(fd);
                    closedir(dir);
                    return 1;
                }

                for(int i = 0; i < no_of_sections; i++)
                {
                    if(read(fd, &sect_name[i], 8) != 8)
                    {
                        printf("ERROR\nreading error\n");
                        close(fd);
                        closedir(dir);
                        return 1;
                    }
                    if(read(fd, &sect_type[i], 1) != 1)
                    {
                        printf("ERROR\nreading error\n");
                        close(fd);
                        closedir(dir);
                        return 1;
                    }
                    else if(sect_type[i] != 16 && sect_type[i] != 80)
                    {
                        // printf("ERROR\nwrong sect_types\n");
                        valid = 0;
                        close(fd);
                        closedir(dir);
                        return 1;
                    }

                    if(read(fd, &sect_offset[i], 4) != 4)
                    {
                        printf("ERROR\nreading error\n");
                        close(fd);
                        closedir(dir);
                        return 1;
                    }
                    if(read(fd, &sect_size[i], 4) != 4)
                    {
                        printf("ERROR\nreading error\n");
                        close(fd);
                        closedir(dir);
                        return 1;
                    }
                }

                if(valid == 1)
                {
                    for(int i = 0; i < no_of_sections; i++)
                        if(sect_size[i] > 1116)
                        {
                            valid = 0;
                            break;
                        }
                }

                close(fd);

                if(valid == 1)
                    printf("%s\n", full_path);
            }
        }
    }
    closedir(dir);
    return 0;
}

int main(int argc, char *argv[])
{
    // argv[0] - program
    // argv[1] - command

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ variant

    if(strcmp(argv[1], "variant") == 0)
        printf("75415\n");

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ end of variant

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ list

    else if(strcmp(argv[1], "list") == 0) // path is mandatory, option and recursion are not
    {
        char* path = (char*)malloc(517 * sizeof(char));
        char* parsed_path = (char*)malloc(512 * sizeof(char));
        path[0] = '\0';

        char* name_filter = (char*)malloc(527 * sizeof(char));
        char* parsed_name_filter = (char*)malloc(512 * sizeof(char));
        name_filter[0] = '\0';

        int execute_filter = 0, recursive = 0;

        for(int i = 2; i < argc; i++) // parse
        {
            if(strstr(argv[i], "path=") != NULL)
            {
                snprintf(path, 517, "%s", argv[i]);
                for(int i = 5; i <= 517; i++)
                    parsed_path[i - 5] = path[i];
            }

            else if(strstr(argv[i], "name_ends_with=") != NULL)
            {
                snprintf(name_filter, 527, "%s", argv[i]);
                for(int i = 15; i <= 527; i++)
                    parsed_name_filter[i - 15] = name_filter[i];
            }
            else if(strcmp(argv[i], "has_perm_execute") == 0)
                execute_filter = 1;

            else if(strcmp(argv[i], "recursive") == 0)
                recursive = 1;
        }

        free(path);
        free(name_filter);

        int results = 0;
        traverse(parsed_path, parsed_name_filter, execute_filter, recursive, &results);

        free(parsed_path);
        free(parsed_name_filter);
    }

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ end of list

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ parse

    else if(strcmp(argv[1], "parse") == 0)
    {
        char* path = (char*)malloc(517 * sizeof(char));
        char* parsed_path = (char*)malloc(512 * sizeof(char));
        path[0] = '\0';
        snprintf(path, 517, "%s", argv[2]);
        for(int i = 5; i < 517; i++)
            parsed_path[i - 5] = path[i];
        free(path);

        int fd = -1;
        fd = open(parsed_path, O_RDONLY);
        if(fd == -1)
        {
            printf("ERROR\ninvalid file\n");
            return 1;
        }

        free(parsed_path);

        char magic[3];
        int header_size, version = 0, no_of_sections = 0;
        char sect_name[14][9] = {0};
        int sect_type[14] = {0}, sect_offset[14] = {0}, sect_size[14] = {0};

        lseek(fd, 0, SEEK_SET);
        if(read(fd, magic, 2) != 2)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }
        if(read(fd, &header_size, 2) != 2)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }
        if(read(fd, &version, 2) != 2)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }
        if(read(fd, &no_of_sections, 1) != 1)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }

        if(strcmp(magic, "Gr") != 0)
        {
            printf("ERROR\nwrong magic\n");
            close(fd);
            return 1;
        }
        else if(version < 83 || version > 185)
        {
            printf("ERROR\nwrong version\n");
            close(fd);
            return 1;
        }
        else if(no_of_sections < 8 || no_of_sections > 14)
        {
            printf("ERROR\nwrong sect_nr\n");
            close(fd);
            return 1;
        }

        for(int i = 0; i < no_of_sections; i++)
        {
            if(read(fd, &sect_name[i], 8) != 8)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            if(read(fd, &sect_type[i], 1) != 1)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            else if(sect_type[i] != 16 && sect_type[i] != 80)
            {
                printf("ERROR\nwrong sect_types\n");
                close(fd);
                return 1;
            }

            if(read(fd, &sect_offset[i], 4) != 4)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            if(read(fd, &sect_size[i], 4) != 4)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
        }
        printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, no_of_sections);
        for(int i = 0; i < no_of_sections; i++)
            printf("section%d: %s %d %d\n", i + 1, sect_name[i], sect_type[i], sect_size[i]);

        close(fd);
    }

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ end of parse

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ extract

    else if(strcmp(argv[1], "extract") == 0)
    {
        char* path = (char*)malloc(517 * sizeof(char));
        char* parsed_path = (char*)malloc(512 * sizeof(char));
        path[0] = '\0';
        snprintf(path, 517, "%s", argv[2]);
        for(int i = 5; i < 517; i++)
            parsed_path[i - 5] = path[i];

        free(path);

        char* section = (char*)malloc(19 * sizeof(char));
        char* section2 = (char*)malloc(11 * sizeof(char));
        section[0] = '\0';
        snprintf(section, 19, "%s", argv[3]);
        for(int i = 8; i <= 19; i++)
            section2[i - 8] = section[i];
        int parsed_section = atoi(section2);

        char* line = (char*)malloc(16 * sizeof(char));
        char* line2 = (char*)malloc(11 * sizeof(char));
        line[0] = '\0';
        snprintf(line, 16, "%s", argv[4]);
        for(int i = 5; i <= 19; i++)
            line2[i - 5] = line[i];
        int parsed_line = atoi(line2);

        free(section); free(section2);
        free(line); free(line2);

        int fd = -1;
        fd = open(parsed_path, O_RDONLY);
        if(fd == -1)
        {
            printf("ERROR\ninvalid file\n");
            return 1;
        }

        free(parsed_path);

        char magic[3];
        int header_size, version = 0, no_of_sections = 0;
        char sect_name[14][9] = {0};
        int sect_type[14] = {0}, sect_offset[14] = {0}, sect_size[14] = {0};

        lseek(fd, 0, SEEK_SET);
        if(read(fd, magic, 2) != 2)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }
        if(read(fd, &header_size, 2) != 2)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }
        if(read(fd, &version, 2) != 2)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }
        if(read(fd, &no_of_sections, 1) != 1)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }

        if(strcmp(magic, "Gr") != 0)
        {
            //printf("ERROR\nwrong magic\n");
            close(fd);
            return 1;
        }
        else if(version < 83 || version > 185)
        {
            //printf("ERROR\nwrong version\n");
            close(fd);
            return 1;
        }
        else if(no_of_sections < 8 || no_of_sections > 14)
        {
            //printf("ERROR\nwrong sect_nr\n");
            close(fd);
            return 1;
        }

        for(int i = 0; i < no_of_sections; i++)
        {
            if(read(fd, &sect_name[i], 8) != 8)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            if(read(fd, &sect_type[i], 1) != 1)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            else if(sect_type[i] != 16 && sect_type[i] != 80)
            {
                //printf("ERROR\nwrong sect_types\n");
                close(fd);
                return 1;
            }

            if(read(fd, &sect_offset[i], 4) != 4)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            if(read(fd, &sect_size[i], 4) != 4)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
        }
        if(parsed_section > no_of_sections)
        {
            printf("ERROR\ninvalid section\n");
            close(fd);
            return 1;
        }


        int counter = 0, counter2 = 0; // counters for lines and characters
        lseek(fd, sect_offset[parsed_section - 1] + sect_size[parsed_section - 1] - 1, SEEK_SET); // the first section is actually the last
        char c;
        for(;;)
        {
            int r = read(fd, &c, 1);
            if(r < 0)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            else if(counter2 > sect_size[parsed_section - 1] - 1 || counter == parsed_line ) // -1 is current
                break;
            else
            {
                counter2++;
                if(c == 10) // 0A
                    counter++;
            }
            lseek(fd, -2, SEEK_CUR);
            
        }

        if(parsed_line > counter + 1)
        {
            printf("ERROR\ninvalid line\n");
            close(fd);
            return 1;
        }
        printf("SUCCESS\n");

        // bug fix
        if(read(fd, &c, 1) < 0)
        {
            printf("ERROR\nreading error\n");
            close(fd);
            return 1;
        }
        else if(c != 10)
            lseek(fd, -1, SEEK_CUR);
        // end of bug fix

        for(;;)
        {
            int r = read(fd, &c, 1);
            
            if(r < 0)
            {
                printf("ERROR\nreading error\n");
                close(fd);
                return 1;
            }
            else if(r == 0)
                break;
            else
            {
                
                if(c == 10 || c == 0) // '0' is not a printable character
                    break;
                printf("%c", c);
            }
        }
        printf("\n");
        close(fd);
    }

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ end of extract

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ findall
    
    else if(strcmp(argv[1], "findall") == 0)
    {
        char* path = (char*)malloc(517 * sizeof(char));
        char* parsed_path = (char*)malloc(512 * sizeof(char));
        path[0] = '\0';
        snprintf(path, 517, "%s", argv[2]);
        for(int i = 5; i < 517; i++)
            parsed_path[i - 5] = path[i];
        free(path);

        int results = 0;
        traverse2(parsed_path, &results);

        free(parsed_path);
    }

    // _,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,__,.-'~'-.,_ end of findall

    return 0;
}
