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

// FIXED - there's a bug in the skeleton code that it doesn't check the argument count
// breadth-first, preorder - output nodes as you visit them during traversal
// fzip -a hello.txt // (prints to console the path of hello.txt.fzip)
// fzip -a lunches
// extra credit opportunity if we use lossless compression / huffman coding?

/*
 *
 * Use seekg when using the C++ IOstreams library. seekp is no use here, since it sets the put pointer.
 * The difference between the various seek functions is just the kind of file/stream objects on which they operate.
 * On Linux, seekg and fseek are probably implemented in terms of lseek
 * Use fseek when using the C stdio library. Use lseek when using low-level POSIX file descriptor I/O.
 *
 * seekg:  http://www.cplusplus.com/reference/istream/istream/seekg/
 * fseek: http://www.cplusplus.com/reference/cstdio/fseek/
 * lseek: http://codewiki.wikidot.com/c:system-calls:lseek



 * Use fseek when using the C stdio library. Use lseek when using low-level POSIX file descriptor I/O. The difference
 * between the various seek functions is just the kind of file/stream objects on which they operate. On Linux, seekg
 * and fseek are probably implemented in terms of lseek .
 *
 */
// open, close, read, write, lseek, dup, dup2 - when you read it moves the file pointer, or you can lseek
// fstat() lists directories, fdopen
// do not do text mode reading.... read in binary, write in binary.
// fseek(fp, -30, SEEK_CUR); // seek backward 30 bytes from the current pos
// fseek(fp, -10, SEEK_END); // seek to the 10th byte before the end of file
// fseek(fp, 0, SEEK_SET);   // seek to the beginning of the file
// rewind(fp);               // seek to the beginning of the file

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
        // need to write file type, file name size, file name here..
        int pathLength = strlen(path);
        write(output_fd, &dirType, sizeof(int));
        write(output_fd, &pathLength, sizeof(int));
        write(output_fd, path, pathLength + 1);
        //cout << "dirType = " << dirType << ". pathLength = " << pathLength << ". path = " << path << endl;

        while ((entry = readdir(directory)) != NULL) {
            char * fqfileName;
            fqfileName = concat(concat(path, "/"), (char *) entry->d_name);
            int fileNameSize = strlen(fqfileName);

            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                //cout << fqfileName;
                if (entry->d_type == DT_REG) {
                    // need to write file type, file name size, file name, file size, file contents here..
                    write(output_fd, &fileType, sizeof(int));
                    write(output_fd, &fileNameSize, sizeof(int));
                    write(output_fd, fqfileName, sizeof(fqfileName) + 1);

                    //cout << "\tregular";
                    //cout << "\t" << entry->d_namlen;
                    input_fd = open(fqfileName, O_RDONLY);
                    if (input_fd == -1) {
                        perror("open");
                        result = false;
                    }

                    // seems to work for both binary and ascii
                    while((ret_in = read (input_fd, &buffer, BUF_SIZE)) > 0){
                        ret_out = write (output_fd, &buffer, (ssize_t) ret_in);
                        if(ret_out != ret_in){
                            perror("write");
                            return 4;
                        }
                    }

                    close(input_fd);
                    //cout << endl;
                } else if (entry->d_type == DT_DIR) {
                    //write(output_fd, &dirType, sizeof(int));
                    //write(output_fd, fqfileName, sizeof(fqfileName) + 1);
                    //cout << "\tdirectory";
                    //cout << endl;
                    //cout << "fqfileName == " << fqfileName << endl;
                    recursiveDir(fqfileName, output_fd);
                } else {
                    //cout << "\tunknown";
                }
            } // we ignore "." and ".." entries in directories
        }
    } else if (errno == ENOTDIR) {
        printf("%s is a standard file\n", path);
        input_fd = open(path, O_RDONLY);
        if (input_fd == -1) {
            perror("open");
            result = false;
        }
        // write out what we need to see..
        int fileNameSize = strlen(path);
        write(output_fd, &fileType, sizeof(int));
        write(output_fd, &fileNameSize, sizeof(int));
        write(output_fd, path, sizeof(path) + 1);

        while((ret_in = read (input_fd, &buffer, BUF_SIZE)) > 0){
            ret_out = write (output_fd, &buffer, (ssize_t) ret_in);
            if(ret_out != ret_in){
                perror("write");
                return 4;
            }
        }
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
        int intGrabber;
        while((ret_in = read (input_fd, &intGrabber, sizeof(int))) > 0) { // ret_in == number of bytes read
            char * fileName;
            if (intGrabber == 0) { // directories extract successfully..
                ret_in = read(input_fd, &intGrabber, sizeof(int));
                fileName = (char *) malloc(intGrabber + 1);
                ret_in = read(input_fd, fileName, intGrabber + 1);
                mkdir(fileName, 0744);
            } else if (intGrabber == 1) { // file
                ret_in = read(input_fd, &intGrabber, sizeof(int));
                fileName = (char *) malloc(intGrabber + 1);
                //cout << "file name size should be = " << intGrabber << endl;
                ret_in = read(input_fd, fileName, intGrabber + 1);
                //cout << "file name should be = " << fileName << endl;
                ret_in = read(input_fd, &intGrabber, sizeof(int));

                //cout << "intGrabber = " << intGrabber << endl;

                char buffer[intGrabber]; // this could be an issue if we're dealing with huge file sizes..
                ret_in = read (input_fd, &buffer, intGrabber);
                //cout << "bytes read for file = " << ret_in << endl;

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
        //	this should be the same as the `path` var above but
        //	without the .fzip
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
