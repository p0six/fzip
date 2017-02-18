#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <cstring>
#include <dirent.h>
using namespace std;

// TODO: Decide on whether or not I want to use return values.. not really using 'bool' in its current state.
//

//You must fill out your name and id below
char * studentName = (char *) "Michael Romero";
char * studentCWID = (char *) "890228026";

//Do not change this section in your submission
char * usageString =
        (char *) "To archive a file: 		fzip -a FILE_PATH\n"
                "To archive a directory: 	fzip -a DIR_PATH\n"
                "To extract a file: 		fzip -x FILE_PATH\n"
                "To extract a directory: 	fzip -x DIR_PATH\n";

bool isExtract = false;
char * parseArg(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Incorrect arguments\n%s", usageString);
        exit(EXIT_FAILURE);
    }
    if (strncmp("-n", argv[1], 2) == 0) {
        printf("Student Name: %s\n", studentName);
        printf("Student CWID: %s\n", studentCWID);
        exit(EXIT_SUCCESS);
    }
    if (strncmp("-a", argv[1], 2) == 0) {
        isExtract = false;
    } else if (strncmp("-x", argv[1], 2) == 0) {
        isExtract = true;
    } else {
        fprintf(stderr, "Incorrect arguements\n%s", usageString);
        exit(EXIT_FAILURE);
    }
    return argv[2];
}
//END OF: Do not change this section

const char * concat(const char * x, const char * y) {
    char * concatenation;
    concatenation = (char *) malloc(strlen(x) + strlen(y) + 1);
    strcpy(concatenation, x);
    strcat(concatenation, y);
    return concatenation;
}

bool addFileContents(const char * path, int output_fd, int fileType, int fileNameSize) {
    // need to write file type, file name size, file name, file size, file contents here..
    bool result = false;
    size_t ret_in, ret_out;
    write(output_fd, &fileType, sizeof(int));
    write(output_fd, &fileNameSize, sizeof(int));
    write(output_fd, path, (size_t) fileNameSize + 1);

    int input_fd = open(path, O_RDONLY);
    if (input_fd == -1) {
        perror("open");
    } else {
        // need to find file size, write it to output_fd
        ssize_t input_file_size = lseek(input_fd, 0, SEEK_END);
        lseek(input_fd,0,SEEK_SET);
        write(output_fd, &input_file_size, sizeof(size_t));

        // need to write only file size
        char * buffer;
        buffer = (char*) malloc(input_file_size + 1);
        ret_in = read(input_fd, buffer, input_file_size);
        ret_out = write(output_fd, buffer, (size_t) ret_in);
        if (ret_out != ret_in) {
            perror("write");
        } else {
            result = true;
        }
    }
    close(input_fd);
    return result;
}

bool recursiveDir(const char * path, int output_fd) {
    bool result = false;
    int dirType = 0;
    int fileType = 1;
    size_t ret_in, ret_out; // these are for debugging, if necessary..
    DIR* directory = opendir(path);
    struct dirent* entry = nullptr;

    if (directory != NULL) {
        int pathLength = strlen(path);
        write(output_fd, &dirType, sizeof(int));
        write(output_fd, &pathLength, sizeof(int));
        write(output_fd, path, pathLength + 1);
        while ((entry = readdir(directory)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                const char * fqFileName;
                fqFileName = concat(concat(path, "/"), (char *) entry->d_name);
                int fileNameSize = strlen(fqFileName);
                if (entry->d_type == DT_REG) {
                    addFileContents(fqFileName, output_fd, fileType, fileNameSize); // maybe do something with bool
                } else if (entry->d_type == DT_DIR) {
                    recursiveDir(fqFileName, output_fd); // maybe do something with bool
                }
            }
        }
        result = true;
    } else if (errno == ENOTDIR) {
        printf("%s is a standard file\n", path); // assuming it's standard if not dir..
        int pathLength = strlen(path);
        addFileContents(path, output_fd, fileType, pathLength);
        result = true;
    } else if (errno == EACCES) {
        printf("Permission issue, or File Not Found\n");
    } else {
        printf("Strange error likely related to lack of resources\n");
    }
    return result;
}

bool archiveFile(char * path) {
    bool result = false;
    int output_fd;
    output_fd = open(concat(path, ".fzip"), O_WRONLY | O_CREAT, 0644);
    if (output_fd == -1) {
        perror("open");
        return false;
    } else {
        recursiveDir(path, output_fd);
        result = true;
    }
    close(output_fd);
    return result;
}

bool extractFile(char * path) {
    bool result = false;
    int input_fd;
    size_t ret_in, ret_out; // used for debugging, if necessary..
    input_fd = open(path, O_RDONLY);
    if (input_fd == -1) {
        perror("open");
    } else {
        int fileType, fileNameSize; // 0 is a dir, 1 is a file
        while((ret_in = read (input_fd, &fileType, sizeof(int))) > 0) { // ret_in == number of bytes read
            // grab the size of the file name
            read(input_fd, &fileNameSize, sizeof(int));

            // grab the actual filename
            char * fileName;
            fileName = (char *) malloc(fileNameSize + 1);
            read(input_fd, fileName, fileNameSize + 1);

            if (fileType == 0) { // directories extract successfully..
                if (mkdir(fileName, 0744) < 0) {
                    perror("write");
                } else {
                    result = true;
                }
            } else if (fileType == 1) { // file
                size_t fileSize;
                read(input_fd, &fileSize, sizeof(size_t)); // intGrabber = size of file

                // we need to read the contents of the file entry within the archive..
                char * buffer; // this could be an issue if we're dealing with huge file sizes..
                buffer = (char*) malloc(fileSize + 1);
                read (input_fd, buffer, fileSize); // buffer = file content

                // now we need to write the contents
                int output_fd = open(fileName, O_WRONLY | O_CREAT, 0644);
                if (output_fd == -1) {
                    perror("open");
                } else {
                    write(output_fd, buffer, fileSize);
                    close(output_fd);
                    result = true;
                }
            }
        }
    }
    close(input_fd);
    return result;
}

int main(int argc, char** argv) {
    char * path = parseArg(argc, argv);
    if (isExtract) {
        printf("Extracting %s\n", path);
        //TODO: your code to start extracting.
        char *outputPath = (char *) ""; 	//the path to the file or directory extracted
        extractFile(path);
        printf("%s\n", outputPath);//relative or absolute path
    } else {
        printf("Archiving %s\n", path);
        //TODO: your code to start archiving.
        archiveFile(path);
        printf("%s.fzip\n", path);
    }
    return EXIT_SUCCESS;
}
