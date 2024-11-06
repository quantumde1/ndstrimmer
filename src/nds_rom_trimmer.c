#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXTENSION  ".nds"
#define BUFFER_SIZE 4096
#define MAX_LENGTH  256 
#define MB_SIZE     1048576
#define KB_SIZE     1024

void help();
void print_info(int filesize, int realsize);
int getFileSize(FILE* stream);
int trim(FILE* strin, const char* filein, int filesize);
int mindex(const char* haystack, const char* needle);

int main(int argc, char* argv[]) {
    FILE* stream;
    char buffer[BUFFER_SIZE];
    char *pos;
    int filesize, fpos, total = 0; 
    int realsize = 0, count = 0;
    const char *filename;

    if (argc < 2) {
        help();
    }
    filename = argv[1];

    if (!strstr(filename, EXTENSION)) {
        help();
    }

    stream = fopen(filename, "rb");
    if (stream == NULL) {
        perror("Couldn't open file");
        exit(EXIT_FAILURE);
    }

    filesize = getFileSize(stream);
    if (filesize == 0) {
        fprintf(stderr, "Error while reading %s: filesize is 0\n", filename);
        fclose(stream);
        exit(EXIT_FAILURE);
    }

    fpos = ftell(stream);
    fseek(stream, filesize - (BUFFER_SIZE - 1), SEEK_SET);

    // Start scanning backwards
    printf("Trimming %s (%dMB %dKB) to...", filename, (filesize / MB_SIZE), (filesize % KB_SIZE) / 1024);
    while (fpos >= 0) {
        size_t bytesRead = fread(buffer, sizeof(char), BUFFER_SIZE - 1, stream);
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        pos = buffer; count = 0;

        // Sum up all the chunks equal to 0xFF (padding bytes)
        while (pos != NULL && *pos != '\0') {
            if (*pos != 0xFF) {
                count++;
            }
            pos++;
        }

        // If there was at least a chunk not equal to 0xFF
        if (count != (BUFFER_SIZE - 1)) {
            fseek(stream, count, SEEK_CUR);
            realsize = ftell(stream);
            break;
        }

        // Move back to new piece of data about to read
        total += BUFFER_SIZE - 1;
        fseek(stream, filesize - total, SEEK_SET);
        fpos = ftell(stream);
    }

    if (realsize > filesize) {
        realsize = filesize;
    }

    print_info(filesize, realsize);

    // Trim file to new file
    trim(stream, filename, realsize);

    fclose(stream);
    return 0;
}

void help() {
    fprintf(stderr, "Usage: nds-rom-trimmer <filename.nds>\n");
    exit(EXIT_FAILURE);
}

int getFileSize(FILE* stream) {
    fseek(stream, 0, SEEK_END);
    int filesize = ftell(stream);
    fseek(stream, 0    , SEEK_SET);
    return filesize;
}

/**
 * Get rid of the whatever data in the file after filesize, storing meaningful data (that means from 0 to filesize) in a new file
 *
 */ 
int trim(FILE* strin, const char* filein, int filesize) {
    FILE* strout;
    char fileout[MAX_LENGTH];
    char buffer[BUFFER_SIZE];
    int pos, fpos, newsize;

    pos = mindex(filein, EXTENSION);
    strncpy(fileout, filein, pos);
    fileout[pos] = '\0';
    strcat(fileout, "_trim");
    strcat(fileout, EXTENSION);

    strout = fopen(fileout, "wb");
    if (!strout) {
        perror("Couldn't open output file for writing");
        exit(EXIT_FAILURE);
    }

    fseek(strin, 0, SEEK_SET);
    for (fpos = 0; fpos < filesize; fpos += BUFFER_SIZE) {
        size_t bytesRead = fread(buffer, sizeof(char), BUFFER_SIZE, strin);
        fwrite(buffer, sizeof(char), bytesRead, strout);
    }

    newsize = ftell(strout);
    fclose(strout);

    printf("%s (%dMB %dKB)\n", fileout, (newsize / MB_SIZE), (newsize % KB_SIZE) / 1024);

    return newsize;
}

int mindex(const char* haystack, const char* needle) {
    const char *pos = strstr(haystack, needle);
    if (pos != NULL) {
        return pos - haystack; // Return the index of the found substring
    }
    return -1; // Return -1 if not found
}

/**
 * Prints summary information about how much the file was trimmed
 *
 */
void print_info(int filesize, int realsize) {
    printf("Trimmed from %d to %d\n", filesize, realsize);
}
