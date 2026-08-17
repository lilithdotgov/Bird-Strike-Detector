#include <stdio.h>
#include <dirent.h>

#include "pico/stdlib.h"
#include "pico/rand.h"

#include "pfs.h"
#include "lfs.h"
#include "ffs_pico.h"

#include "accelerometer.h"
#include "storage.h"

#define ROOT_OFFSET  0x100000
#define ROOT_SIZE    0x100000 // 1 MB of size
#define MAX_DIR_SIZE 200      // Likely can't hold even 100 samples, so this should suffice

struct pfs_pfs *pfs;
struct lfs_config cfg;

FILE *fp;

// Initialize the file system
void initialize_lfs(void) {
    ffs_pico_createcfg(&cfg, ROOT_OFFSET, ROOT_SIZE);
    pfs = pfs_ffs_create(&cfg);
    if (pfs_mount(pfs, "/") != 0) { // If cannot mount lfs, we must format it
        printf("Mount failed. Formatting LittleFS...\n");
        fflush(stdout);

        lfs_t lfs;
        if (lfs_format(&lfs, &cfg) == 0) {
            printf("Format successful! Mounting...\n");
            pfs_mount(pfs, "/");
        } else {
            printf("Error: Failed to format LittleFS!\n");
        }
    }
}

void write_file(char *name, uint8_t *data) { // Used for debugging currently
    fp = fopen(name, "w");
    if (fp != NULL) {
        for (int i = 0; i < STRIKE_SAMPLES * BPS; i += BPS) {
            // Combine LSB (data[i]) and MSB (data[i+1]), then cast to signed 16-bit integer
            double x = G * (double)(int16_t)((data[i + 1] << 8) | data[i]) / 256.0;
            double y = G * (double)(int16_t)((data[i + 3] << 8) | data[i + 2]) / 256.0;
            double z = G * (double)(int16_t)((data[i + 5] << 8) | data[i + 4]) / 256.0;

            fprintf(fp, "X: %f\t Y: %f\t Z: %f\n", x, y, z); // Write line to file
        }
    } else {
        printf("Error: Failed to open file\n");
    }
    fflush(fp);
    fclose(fp);
}

void write_binary_file(char *name, uint8_t *data) {
    fp = fopen(name, "wb");
    if (fp != NULL) {
        int16_t buf[3];
        for (int i = 0; i < STRIKE_SAMPLES * BPS; i += BPS) {
            // Combine LSB (data[i]) and MSB (data[i+1]), then cast to signed 16-bit integer
            buf[0] = G * (int16_t)((data[i + 1] << 8) | data[i]);
            buf[1] = G * (int16_t)((data[i + 3] << 8) | data[i + 2]);
            buf[2] = G * (int16_t)((data[i + 5] << 8) | data[i + 4]);

            fwrite(buf, sizeof(int16_t), 3, fp);
        }
    } else {
        printf("Error: Failed to open file\n");
    }
    fflush(fp);
    fclose(fp);
}

void print_file(char *name) { // Not meant for binary file
    fp = fopen(name, "r");
    if (fp != NULL) {
        double x, y, z;
        printf("Data:\n");
        while (fscanf(fp, "X: %lf\t Y: %lf\t Z: %lf\n", &x, &y, &z) == 3) {
            printf("X: %f\t Y: %f\t Z: %f\n", x, y, z);
        }
    } else {
        printf("Error: failed to open file");
    }
    fflush(stdout);
    fclose(fp);
}

void read_binary_file(char *name, int16_t *buf) { // Outputs contents of file to supplied buffer
    fp = fopen(name, "rb");
    if (fp != NULL) {
        int16_t x, y, z;
        while (fscanf(fp, "%d%d%d", &x, &y, &z) == 3) {
            fread(buf, sizeof(int16_t), 3, fp);
        }
    } else {
        printf("Error: failed to open file");
    }
    fflush(fp);
    fclose(fp);
}

void print_dir(void) {
    DIR *dp = opendir("/");
    if (dp == NULL) { // Ensure we can read directory
        printf("Error: Could not open directory\n");
    } else {
        printf("Files:\n");
        struct dirent *ep;
        while ((ep = readdir(dp)) != NULL) {
            printf("  %s    Type = %d\n", ep->d_name, ep->d_type);
            fflush(stdout);
        }
        closedir(dp);
    }
    printf("End of file search!\n");
    fflush(stdout);
}

int list_dir(char **dir_files) { // Writes to an array of points to strings, terminated by a NULL pointer
    DIR *dp = opendir("/");
    if (dp == NULL) { // Ensure we can read directory
        printf("Error: Could not open directory\n");
        return -1;
    } else {
        struct dirent *ep;

        int i = 0;
        while ((ep = readdir(dp)) != NULL) {
            dir_files[i] = ep->d_name;
            i++;
        }
        dir_files[i] = NULL; // Append with NULL to signal termination

        closedir(dp);
        return 0;
    }
}

int find_in_dir(char *str, char **return_files) { // Writes to an array of points to strings, terminated by a NULL pointer, which all contain the given string
    char *dir_files[MAX_DIR_SIZE];

    if (!list_dir(dir_files)) { // ensure function executed properly
        return -1;
    }
    int j = 0;
    for (int i = 0; dir_files[i] != NULL; i++) {
        if (strstr(dir_files[i], str) != NULL) { // strstr returns pointer to first occurence of str, otherwise NULL
            return_files[j] = dir_files[i];
            j++;
        }
    }
    return_files[j] = NULL;
    return 0;
}

// With how we have redone NTP this might no longer be necessary, instead just have a normal naming function
char *generate_temp_bin_name(void) { // Returns a random string of numbers as a temporary file name, ensures no collisions, all equal length, must be freed!
    uint16_t rand;
    char *name = (char *)malloc(sizeof(char) * (1 + 5 + 4 + 1)); // "/" + 5 digits + ".bin" + terminator
    char *dir_files[MAX_DIR_SIZE];

    for (;;) {
        rand = (uint16_t)get_rand_32();                       // Generate new 16 bit number
        snprintf(name, 10, "/%d.bin", rand);                  // Create name with random number
        if ((rand > 10000) && find_in_dir(name, dir_files)) { // If it's 5 digits and not in usage then it's a valid name
            return name;
        }
    }
}