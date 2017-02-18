#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <cstring>
#include <dirent.h>
#include <iostream>
using namespace std;

// TODO: Decide on whether or not I want to use return values.. not really using 'bool' in its current state.
//

//You must fill out your name and id below
char * studentName = (char *) "Michael Romero";
char * studentCWID = (char *) "890228026";

//Do not change this section in your submission
char * usageString =
        (char *) "To archive a file: 		fzip -a INPUT_FILE_NAME  OUTPUT_FILE_NAME\n"
                "To archive a directory: 	fzip -a INPUT_DIR_NAME   OUTPUT_DIR_NAME\n"
                "To extract a file: 		fzip -x INPUT_FILE_NAME  OUTPUT_FILE_NAME\n"
                "To extract a directory: 	fzip -x INPUT_DIR_NAME   OUTPUT_DIR_NAME\n";

bool isExtract = false;
void parseArg(int argc, char *argv[], char ** inputName, char ** outputName) {
    if (argc >= 2 && strncmp("-n", argv[1], 2) == 0) {
        printf("Student Name: %s\n", studentName);
        printf("Student CWID: %s\n", studentCWID);
        exit(EXIT_SUCCESS);
    }

    if (argc != 4) {
        fprintf(stderr, "Incorrect arguements\n%s", usageString);
        exit(EXIT_FAILURE);
    }

    *inputName  = argv[2];
    *outputName = argv[3];
    if (strncmp("-a", argv[1], 2) == 0) {
        isExtract = false;
    } else if (strncmp("-x", argv[1], 2) == 0) {
        isExtract = true;
    } else {
        fprintf(stderr, "Incorrect arguements\n%s", usageString);
        exit(EXIT_FAILURE);
    }
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

bool archiveFile(char * path, char * outputPath) {
    bool result = false;
    int output_fd;
    output_fd = open(outputPath, O_WRONLY | O_CREAT, 0644);
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

bool extractFile(char * path, char * outputPath) {
    bool result = false;
    int input_fd;
    size_t ret_in, ret_out; // used for debugging, if necessary..
    input_fd = open(path, O_RDONLY);
    if (input_fd == -1) {
        perror("open");
    } else {
        if (mkdir(outputPath, 0744) < 0 && strcmp(outputPath, ".") != 0) {
            perror("write");
        } else {
            int fileType, fileNameSize; // 0 is a dir, 1 is a file
            while ((ret_in = read(input_fd, &fileType, sizeof(int))) > 0) { // ret_in == number of bytes read
                // grab the size of the file name
                read(input_fd, &fileNameSize, sizeof(int));

                // grab the actual filename
                char *fileName;
                fileName = (char *) malloc(fileNameSize + 1);
                read(input_fd, fileName, fileNameSize + 1);

                // prepend outputPath with fileName per new requirements
                const char *extractedFile;
                extractedFile = concat(concat(outputPath, "/"), (char *) fileName);

                if (fileType == 0) { // directories extract successfully..
                    if (mkdir(extractedFile, 0744) < 0) {
                        perror("write");
                    } else {
                        result = true;
                    }
                } else if (fileType == 1) { // file
                    size_t fileSize;
                    read(input_fd, &fileSize, sizeof(size_t)); // intGrabber = size of file

                    // we need to read the contents of the file entry within the archive..
                    char *buffer; // this could be an issue if we're dealing with huge file sizes..
                    buffer = (char *) malloc(fileSize + 1);
                    read(input_fd, buffer, fileSize); // buffer = file content

                    // now we need to write the contents
                    int output_fd = open(extractedFile, O_WRONLY | O_CREAT, 0644);
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
    }
    close(input_fd);
    return result;
}

/**
 *
 * Your program should archive or extract based on the flag passed in.
 * Both when extracting and archiving, it should print the output file/dir path as the last line.
 *
 * @param argc the number of args
 * @param argv the arg string table
 * @return
 */

int main(int argc, char** argv) {
    char * inputName, * outputName;
    parseArg(argc, argv, &inputName, &outputName);
    if (isExtract) {
        printf("Extracting %s\n", inputName);
        //TODO: your code to start extracting.
        extractFile(inputName, outputName);
        printf("%s\n", outputName);//relative or absolute path
    } else {
        printf("Archiving %s\n", inputName);
       //TODO: your code to start archiving.
        archiveFile(inputName, outputName);
        printf("%s\n", outputName);
    }
    return EXIT_SUCCESS;
}
