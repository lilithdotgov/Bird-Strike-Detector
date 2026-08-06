#include <stdio.h>
#include <dirent.h>

#include "pico/stdlib.h"

#include "pfs.h"
#include "lfs.h"
#include "ffs_pico.h"

#include "accelerometer.h"
#include "storage.h"

#define ROOT_OFFSET 0x100000
#define ROOT_SIZE   0x100000 // 1 MB of size

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

void write_file(char *name, uint8_t *data) {
    fp = fopen(name, "w"); // TODO: Change to "wb" in future
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

void read_file(char *name) {
    fp = fopen(name, "r"); // TODO: Change to "rb" in future
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

void list_dir(void) {
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