#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
        fileInfo.permissions = fileStat.st_mode & 0777;
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

    printf("The files have been merged: %s\n", archiveName);
}

void extractArchive(char* archiveName, char* directory) {
    FILE* archive = fopen(archiveName, "rb");
    if (archive == NULL) {
        perror("Error opening archive file");
        exit(EXIT_FAILURE);
    }

    // Check if the directory exists
    struct stat directoryStat;
    if (stat(directory, &directoryStat) != 0 || !S_ISDIR(directoryStat.st_mode)) {
        // Create the directory
        if (mkdir(directory, 0777) != 0) {
            perror("Error creating directory");
            exit(EXIT_FAILURE);
        }
    }

    // Read organization section size
    size_t orgSectionSize;
    fscanf(archive, "%010zu|", &orgSectionSize);

    // Read file information
    FileInfo fileInfo;
    while (fscanf(archive, "%[^,],%o,%zu|", fileInfo.filename, &fileInfo.permissions, &fileInfo.size) == 3) {
        // Extract the file to the specified directory
        char outputPath[256];
        snprintf(outputPath, sizeof(outputPath), "%s/%s", directory, fileInfo.filename);

        FILE* output = fopen(outputPath, "wb");
        if (output == NULL) {
            perror("Error creating output file");
            exit(EXIT_FAILURE);
        }

        char buffer[fileInfo.size];
        fread(buffer, 1, fileInfo.size, archive);
        fwrite(buffer, 1, fileInfo.size, output);

        printf("Extracted file: %s\n", fileInfo.filename);

        fclose(output);
    }
    
    // Close the archive
    fclose(archive);

    printf("Files opened in the %s directory.\n", directory);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -b file1 file2 ... -o archive.sau\n", argv[0]);
        fprintf(stderr, "       %s -a archive.sau directory\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-b") == 0) {
        char* archiveName = "a.sau"; // Default archive name
        char** fileNames = &argv[2];
        int fileCount = argc - 4;

        
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

        char* archiveName = argv[2];
        char* directory = argv[3];

        extractArchive(archiveName, directory);
    } else {
        fprintf(stderr, "Invalid option: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
