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

#define BUF_SIZE 8192

using namespace std;

//You must fill out your name and id below
char * studentName = (char *) "Michael Romero";
char * studentCWID = (char *) "890228026";

char * concat(char * x, char * y) {
    char * concatted;
    concatted = (char *) malloc(strlen(x) + strlen(y) + 1);
    strcpy(concatted, x);
    strcat(concatted, y);
    return concatted;
}

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

/**
 *
 * Your program should archive or extract based on the flag passed in.
 * Both when extracting and archiving, it should print the output file/dir path as the last line.
 *
 * @param argc the number of args
 * @param argv the arg string table
 * @return
 */

bool addFileContents(char * path, int output_fd, int fileType, int fileNameSize) {
    // need to write file type, file name size, file name, file size, file contents here..
    bool result = false;
    int ret_in, ret_out;
    write(output_fd, &fileType, sizeof(int));
    write(output_fd, &fileNameSize, sizeof(int));
    write(output_fd, path, fileNameSize + 1);

    int input_fd = open(path, O_RDONLY);
    if (input_fd == -1) {
        perror("open");
        result = false;
    } else {
        // need to find file size, write it to output_fd
        ssize_t input_file_size = lseek(input_fd, 0, SEEK_END);
        lseek(input_fd,0,SEEK_SET);
        ret_out = write(output_fd, &input_file_size, sizeof(int));

        // need to write only file size
        char * buffer;
        buffer = (char*) malloc(input_file_size);
        ret_in = read(input_fd, buffer, input_file_size);
        ret_out = write(output_fd, buffer, (ssize_t) ret_in);
        if (ret_out != ret_in) {
            perror("write");
        }
    }
    close(input_fd);
}

bool recursiveDir(char * path, int output_fd) {
    int input_fd;
    ssize_t ret_in, ret_out;
    DIR* directory = opendir(path);
    struct dirent* entry = nullptr;
    char buffer[BUF_SIZE];
    bool result = false;
    int dirType = 0;
    int fileType = 1;

    if (directory != NULL) {
        int pathLength = strlen(path);
        write(output_fd, &dirType, sizeof(int));
        write(output_fd, &pathLength, sizeof(int));
        write(output_fd, path, pathLength + 1);
        while ((entry = readdir(directory)) != NULL) {
            char * fqfileName;
            fqfileName = concat(concat(path, "/"), (char *) entry->d_name);
            int fileNameSize = strlen(fqfileName);
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                if (entry->d_type == DT_REG) {
                    addFileContents(fqfileName, output_fd, fileType, fileNameSize);
                } else if (entry->d_type == DT_DIR) {
                    recursiveDir(fqfileName, output_fd);
                } // no use for an else at this point..
            } // we ignore "." and ".." entries in directories
        }
    } else if (errno == ENOTDIR) {
        printf("%s is a standard file\n", path);
        int pathLength = strlen(path);
        addFileContents(path, output_fd, fileType, pathLength);
    } else if (errno =  EACCES) {
        printf("Privilege error, or File Not Found\n");
    } else if (errno == EMFILE) {
        printf("The process has too many files open\n");
    } else if (errno = ENFILE) {
        printf("Cannot support additional files at this point\n");
    } else if (errno == ENOMEM) {
        printf("Not enough memory available\n");
    }
    return result;
}

bool archiveFile(char * path) {
    int output_fd;
    ssize_t ret_in, ret_out;
    bool result = true;
    output_fd = open(concat(path, ".fzip"), O_WRONLY | O_CREAT, 0644);
    if (output_fd == -1) {
        perror("open");
        return false;
    }
    recursiveDir(path, output_fd);
    close(output_fd);
}

bool extractFile(char * path) {
    int input_fd;
    ssize_t ret_in;
    input_fd = open(path, O_RDONLY);
    if (input_fd == -1) {
        perror("open");
        return false;
    } else {
        ssize_t input_file_size = lseek(input_fd, 0, SEEK_END);
        lseek(input_fd,0,SEEK_SET);
        int fileType;
        while((ret_in = read (input_fd, &fileType, sizeof(int))) > 0) { // ret_in == number of bytes read
            char * fileName;
            int fileNameSize;
            ret_in = read(input_fd, &fileNameSize, sizeof(int));
            fileName = (char *) malloc(fileNameSize + 1);
            ret_in = read(input_fd, fileName, fileNameSize + 1);
            if (fileType == 0) { // directories extract successfully..
                mkdir(fileName, 0744);
            } else if (fileType == 1) { // file
                int fileSize;
                ret_in = read(input_fd, &fileSize, sizeof(int)); // intGrabber = size of file

                // we need to read the contents of the file entry within the archive..
                char * buffer; // this could be an issue if we're dealing with huge file sizes..
                buffer = (char*) malloc(fileSize + 1);
                ret_in = read (input_fd, buffer, fileSize + 1); // buffer = file content

                // now we need to write the contents
                int output_fd = open(fileName, O_WRONLY | O_CREAT, 0644);
                if (output_fd == -1) {
                    perror("open");
                } else {
                    write(output_fd, buffer, fileSize);
                    close(output_fd);
                }

            }
        }
    }
    close(input_fd);
    return true;
}

int main(int argc, char** argv) {
    char * path = parseArg(argc, argv);
    if (isExtract) {
        printf("Extracting %s\n", path);
        //TODO: your code to start extracting.
        //char *outputPath = (char *) ""; 	//the path to the file or directory extracted
        extractFile(path);
        //printf("%s\n", outputPath);//relative or absolute path
    } else {
        printf("Archiving %s\n", path);
        //TODO: your code to start archiving.
        archiveFile(path);
        printf("%s.fzip\n", path);
    }
    return EXIT_SUCCESS;
}
