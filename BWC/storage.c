#include <stdio.h>
#include <dirent.h>

#include "pico/stdlib.h"

#include "pfs.h"
#include "lfs.h"
#include "ffs_pico.h"

#include "accelerometer.h" // Could possibly remove this via some better code
#include "storage.h"
#include "communication.h"
#include "secrets.h"

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
            buf[0] = (int16_t)((data[i + 1] << 8) | data[i]);
            buf[1] = (int16_t)((data[i + 3] << 8) | data[i + 2]);
            buf[2] = (int16_t)((data[i + 5] << 8) | data[i + 4]);

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
    fclose(fp);
}

void print_binary_file(char *name) { // For debugging
    fp = fopen(name, "rb");
    if (fp != NULL) {
        int16_t buf[3];
        double x, y, z;
        for (int i = 0; i < STRIKE_SAMPLES; i++) {
            fread(buf, sizeof(int16_t), 3, fp);

            x = G * (double)(buf[0]) / 256.0;
            y = G * (double)(buf[1]) / 256.0;
            z = G * (double)(buf[2]) / 256.0;

            printf("X: %f\t Y: %f\t Z: %f\n", x, y, z);
        }
    } else {
        printf("Error: failed to open file");
    }
    fclose(fp);
}

void read_binary_file(char *name, int16_t *buf) { // Outputs contents of file to supplied buffer
    fp = fopen(name, "rb");
    if (fp != NULL) {
        fread(buf, sizeof(int16_t), STRIKE_SAMPLES * 3, fp);
        fclose(fp);
    } else {
        printf("Error: failed to open file");
    }
}

void print_dir(void) {
    DIR *dp = opendir("/");
    if (dp == NULL) { // Ensure we can read directory
        printf("Error: Could not open directory\n");
    } else {
        printf("\n--------------------\nFiles:\n");
        struct dirent *ep;
        while ((ep = readdir(dp)) != NULL) {
            printf("  %s    Type = %d\n", ep->d_name, ep->d_type);
        }
        closedir(dp);
    }
    printf("End of file search!\n--------------------\n\n");
}

// Can be used to list every file in the directory by passing an empty string
int find_in_dir(char *str, char **return_files) { // Writes to an uninitialized array of points to strings, terminated by a NULL pointer, which all contain the given string, returns 0 on success, each string must be freed!
    DIR *dp = opendir("/");
    if (dp == NULL) { // Ensure we can read directory
        printf("Error: Could not open directory\n");
        return -1;
    }

    struct dirent *ep;
    int j = 0;

    while ((ep = readdir(dp)) != NULL) {
        if (strstr(ep->d_name, str) != NULL) {
            // Allocate memory for the string and copy it over safely
            return_files[j] = malloc(strlen(ep->d_name) + 1);
            strcpy(return_files[j], ep->d_name);
            j++;
        }
    }
    return_files[j] = NULL; // Append with NULL to signal termination
    closedir(dp);
    return 0;
}

// With how we have redone NTP this might no longer be necessary, instead just have a normal naming function
char *generate_bin_name(void) { // Returns a string of the filename, all equal length for individual devices, must be freed!
    char mac_buff[MAC_LEN];
    get_mac(mac_buff);
    u_int64_t sntp_time = get_time();
    int name_len        = (1 + sizeof(MICRO_NUM) + 1 + 17 + 1 + 10 + 4 + 1); // "/" + sizeof(Device ID) + "_" + 17 MAC symbols + "_" + 10 NTP digits + ".bin" + terminator
    char *name          = (char *)malloc(sizeof(char) * name_len);

    snprintf(name, name_len, "/%s_%s_%llu.bin", MICRO_NUM, mac_buff, sntp_time);
    return name;
}

// Turns strike data into Base64 as per GitHub API documentation, see https://docs.github.com/en/rest/repos/contents?apiVersion=2026-03-10#create-or-update-file-contents
// Does not support padding! Isn't needed currently since 3 samples each with 16 bits means we need exactly 8 Base64 characters to represent them
void encode_data_to_base64(const int16_t *data, char *data_B64) {
    // clang-format off
    static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                    'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                    '4', '5', '6', '7', '8', '9', '+', '/'};
    //clang-format on
    
    // Collection of 8 Base64 characters + null terminator
    char char_6b[8 + 1];
    int n = 0; //Keeps track of where in the buffer we are when adding the characters

    for (int i = 0; i < STRIKE_SAMPLES * 3; i += 3){
        //Get what each character value should be 
        // C has >> as highest precedence, then &, then |
        //++ as a post-increment is run after using the value of the varibale
        data_B64[n++] = encoding_table[ ( data[i]   & 0b1111110000000000 ) >> 10 ];
        data_B64[n++] = encoding_table[ ( data[i]   & 0b0000001111110000 ) >> 4  ];
        data_B64[n++] = encoding_table[ ( data[i]   & 0b0000000000001111 ) << 2  | ( data[i+1] & 0b1100000000000000 ) >> 14 ];
        data_B64[n++] = encoding_table[ ( data[i+1] & 0b0011111100000000 ) >> 8  ];
        data_B64[n++] = encoding_table[ ( data[i+1] & 0b0000000011111100 ) >> 2  ];
        data_B64[n++] = encoding_table[ ( data[i+1] & 0b0000000000000011 ) << 4  | ( data[i+2] & 0b1111000000000000 ) >> 12 ];
        data_B64[n++] = encoding_table[ ( data[i+2] & 0b0000111111000000 ) >> 6  ];
        data_B64[n++] = encoding_table[ ( data[i+2] & 0b0000000000111111 ) >> 0  ];
    }
    data_B64[n] = '\0';
}