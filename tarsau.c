#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FILES 32
#define MAX_SIZE 200 * 1024 * 1024 // 200 MB

typedef struct {
    char filename[256];
    mode_t permissions;
    size_t size;
} FileInfo;

void createArchive(char* archiveName, int fileCount, char* fileNames[]) {
    // Open the archive file
    FILE* archive = fopen(archiveName, "wb");
    if (archive == NULL) {
        perror("Error creating archive file");
        exit(EXIT_FAILURE);
    }

    // The first 10 bytes hold the numerical size of the first section in ASCII. It's now 0 it will add related size later.
    size_t orgSectionSize = 0;
    fprintf(archive, "%010zu|", orgSectionSize);

    long orgSectionStart = ftell(archive);

    // Write file information
    for (int i = 0; i < fileCount; ++i) {
        // Get file information
        struct stat fileStat;
        if (stat(fileNames[i], &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
            fprintf(stderr, "%s input file format is incompatible!\n", fileNames[i]);
            exit(EXIT_FAILURE);
        }

        FileInfo fileInfo;
        strcpy(fileInfo.filename, fileNames[i]);
        fileInfo.permissions = 0777;
        fileInfo.size = fileStat.st_size;

        // Write file information
        fprintf(archive, "%s,%o,%zu|", fileInfo.filename, fileInfo.permissions, fileInfo.size);

    }
    long orgSectionEnd = ftell(archive);

    long orgSize = ftell(archive) - orgSectionStart +1;

    // Update organization section size
    fseek(archive, 0, SEEK_SET);
    fprintf(archive, "%010zu|", orgSize);
    fseek(archive, orgSectionEnd, SEEK_SET);

    //Write file contents
    for (int i = 0; i < fileCount; ++i) {
        // Get file information
        struct stat fileStat;
        if (stat(fileNames[i], &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
            fprintf(stderr, "%s input file format is incompatible!\n", fileNames[i]);
            exit(EXIT_FAILURE);
        }

        FileInfo fileInfo;
        strcpy(fileInfo.filename, fileNames[i]);
        fileInfo.permissions = 0777;
        fileInfo.size = fileStat.st_size;

        // Write file contents
        FILE* input = fopen(fileNames[i], "rb");
        if (input == NULL) {
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }

        char buffer[fileInfo.size];
        fread(buffer, 1, fileInfo.size, input);
        fwrite(buffer, 1, fileInfo.size, archive);

        fclose(input);
    }


    // Close the archive
    fclose(archive);

    printf("Archive created: %s\n", archiveName);
}

void extractArchive(char* archiveName, char* directory) {
    FILE* archive = fopen(archiveName, "rb");
    if (archive == NULL) {
        perror("Error opening archive file");
        exit(EXIT_FAILURE);
    }

    // Read organization section size
    size_t orgSectionSize;
    fscanf(archive, "%010zu|", &orgSectionSize);

    // Create directory to extract files to
    mkdir(directory, 0777);

    // Read file information
    long orgSectionEnd = ftell(archive) + orgSectionSize - 1; // content's start line

    long location;
    FileInfo fileInfo;

    while (fscanf(archive, "%[^,],%o,%zu|", fileInfo.filename, &fileInfo.permissions, &fileInfo.size)==3) {
        
        location = ftell(archive);

        // Add the directory path to the filename to get the full path
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "%s/%s", directory, fileInfo.filename);

        // Create the file and write the content
        FILE* outputFile = fopen(filePath, "wb");
        if (outputFile == NULL) {
            perror("Error creating output file");
            exit(EXIT_FAILURE);
        }else{
            chmod(filePath, 0777);
        }

        //go to content's start line
        fseek(archive, orgSectionEnd, SEEK_SET);

        orgSectionEnd += fileInfo.size;

        char buffer[fileInfo.size];
        fread(buffer, 1, fileInfo.size, archive);
        fwrite(buffer, 1, fileInfo.size, outputFile);
        
        if(location != orgSectionEnd){
            fseek(archive, location, SEEK_SET);
        }else{
            fclose(archive);
        }
       

        fclose(outputFile);
    }

    // Close the archive
    fclose(archive);

    printf("Files opened in the %s directory.\n", directory);
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s -b file1 file2 ... -o archive.sau\n", argv[0]);
        fprintf(stderr, "       %s -a archive.sau directory\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-b") == 0) {
        char* archiveName = "a.sau"; // Default archive name
        char** fileNames = &argv[2];
        int fileCount = argc - 4;

        if (argc < 4) {
            fprintf(stderr, "Usage: %s -b file1 file2 ... -o archive.sau\n", argv[0]);
            exit(EXIT_FAILURE);
        } 
        

        if (strcmp(argv[argc - 2], "-o") == 0) {
            archiveName = argv[argc - 1];
        } else if (strcmp(argv[argc -1], "-o") == 0) {
            fileCount = argc - 3;
        } else {
            fprintf(stderr, "Invalid option: %s\n", argv[argc - 2]);
            exit(EXIT_FAILURE);
        }
        

        if (fileCount > MAX_FILES) {
            fprintf(stderr, "Maximum number of files exceeded: %d\n", MAX_FILES);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < fileCount; ++i) {
            struct stat fileStat;
            if (stat(fileNames[i], &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
                fprintf(stderr, "%s input file format is incompatible!\n", fileNames[i]);
                exit(EXIT_FAILURE);
            }

            if (fileStat.st_size > MAX_SIZE) {
                fprintf(stderr, "%s input file size exceeded: %d\n", fileNames[i], MAX_SIZE);
                exit(EXIT_FAILURE);
            }
        }

        createArchive(archiveName, fileCount, fileNames);
    } else if (strcmp(argv[1], "-a") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s -a archive.sau directory\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        //check if archive file exists
        FILE* archive = fopen(argv[2], "rb");
        if (archive == NULL) {
            printf("Archive file is inappropriate or corrupt!\n");
            exit(EXIT_FAILURE);
        }

        char* archiveName = argv[2];
        char* directory = argv[3];

        extractArchive(archiveName, directory);
    } else {
        fprintf(stderr, "Invalid option: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return 0;
}

