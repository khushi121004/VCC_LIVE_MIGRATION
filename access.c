#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

// Function to clear soft-dirty bits for a process
int clear_soft_dirty(int pid) {
    char clear_refs_path[64];
    sprintf(clear_refs_path, "/proc/%d/clear_refs", pid);
    FILE *clear_refs = fopen(clear_refs_path, "w");
    if (!clear_refs) {
        perror("Cannot open clear_refs");
        return -1;
    }
    fprintf(clear_refs, "4"); // Clear soft-dirty bits
    fclose(clear_refs);
    return 0;
}


// Structure to track page activity
typedef struct {
    unsigned long addr;
    int access_count;
    int is_hot;
} PageTracker;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <pid> <output_file> <sampling_iterations>\n", argv[0]);
        return 1;
    }
    
    int pid = atoi(argv[1]);
    char *output_file = argv[2];
    int iterations = atoi(argv[3]);
    
    char maps_path[64];
    sprintf(maps_path, "/proc/%d/maps", pid);
    
    char pagemap_path[64];
    sprintf(pagemap_path, "/proc/%d/pagemap", pid);
    
    // Open mem file for reading VM pages later
    char mem_path[64];
    sprintf(mem_path, "/proc/%d/mem", pid);
    int mem_fd = open(mem_path, O_RDONLY);
    if (mem_fd == -1) {
        perror("Cannot open mem file");
        return 1;
    }
    
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        perror("Cannot open output file");
        close(mem_fd);
        return 1;
    }
    
    size_t page_size = getpagesize();
    unsigned char *page_buffer = malloc(page_size);
    
    // First pass: identify all mappable memory regions and create trackers
    FILE *maps = fopen(maps_path, "r");
    if (!maps) {
        perror("Cannot open maps file");
        close(mem_fd);
        fclose(out);
        return 1;
    }
    
    // Count total pages for pre-allocation
    int total_pages = 0;
    char line[256];
    while (fgets(line, sizeof(line), maps)) {
        unsigned long start, end;
        char perms[5];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (perms[0] != 'r') continue;
        
        total_pages += (end - start) / page_size;
    }
    fseek(maps, 0, SEEK_SET);
    
    // Allocate page trackers
    PageTracker *trackers = (PageTracker*)calloc(total_pages, sizeof(PageTracker));
    int tracker_count = 0;
    
    // Now build the page tracker array
    while (fgets(line, sizeof(line), maps)) {
        unsigned long start, end;
        char perms[5];
        char path[256] = "";
        
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255s", &start, &end, perms, path) < 3) continue;
        if (perms[0] != 'r') continue;
        
        // Skip libraries and small regions, focus on likely VM memory
        if ((strstr(path, ".so") || strstr(path, "lib") || strlen(path) < 3) && 
            (end - start) < 1024 * page_size) {  // Skip small library regions
            continue;
        }
        
        // Add pages from this region to tracker
        for (unsigned long addr = start; addr < end; addr += page_size) {
            trackers[tracker_count].addr = addr;
            trackers[tracker_count].access_count = 0;
            trackers[tracker_count].is_hot = 0;
            tracker_count++;
        }
    }
    fclose(maps);
    
    printf("Tracking %d pages across VM memory\n", tracker_count);
    
    // Open pagemap file for reading
    int pagemap_fd = open(pagemap_path, O_RDONLY);
    if (pagemap_fd == -1) {
        perror("Cannot open pagemap file");
        close(mem_fd);
        fclose(out);
        free(trackers);
        return 1;
    }
    
    // Now do multiple sampling iterations
    for (int iter = 0; iter < iterations; iter++) {
        printf("Starting sampling iteration %d/%d\n", iter+1, iterations);
        
        // Clear soft-dirty bits
        if (clear_soft_dirty(pid) != 0) {
            fprintf(stderr, "Failed to clear soft-dirty bits\n");
            break;
        }
        
        // Wait for some activity
        printf("Waiting for page access activity...\n");
        sleep(5);  // 5 second sampling window
        
        // Now check which pages were accessed
        int active_pages = 0;
        for (int i = 0; i < tracker_count; i++) {
            unsigned long addr = trackers[i].addr;
            unsigned long index = addr / page_size;
            unsigned long offset = index * 8;
            
            // Seek to the page entry
            if (lseek(pagemap_fd, offset, SEEK_SET) != offset) {
                continue;
            }
            
            // Read the pagemap entry
            unsigned long entry;
            if (read(pagemap_fd, &entry, 8) != 8) {
                continue;
            }
            
            // Check the soft-dirty bit (bit 55)
            if (entry & (1UL << 55)) {
                trackers[i].access_count++;
                active_pages++;
            }
        }
        
        printf("Iteration %d: %d pages accessed\n", iter+1, active_pages);
    }
    
    close(pagemap_fd);
    
    // Sort pages by access count to identify hot pages
    // For simplicity, just use a threshold: pages accessed in majority of iterations
    int threshold = iterations / 2;
    int hot_pages = 0;
    
    for (int i = 0; i < tracker_count; i++) {
        if (trackers[i].access_count > threshold) {
            trackers[i].is_hot = 1;
            hot_pages++;
        }
    }
    
    printf("Identified %d hot pages out of %d total pages\n", hot_pages, tracker_count);
    
    // Now extract the hot pages
    for (int i = 0; i < tracker_count; i++) {
        if (trackers[i].is_hot) {
            // Seek to the page address
            if (lseek(mem_fd, trackers[i].addr, SEEK_SET) != trackers[i].addr) {
                continue;
            }
            
            // Read the page
            if (read(mem_fd, page_buffer, page_size) != page_size) {
                continue;
            }
            
            // Write the page to output
            fwrite(page_buffer, 1, page_size, out);
        }
    }
    
    printf("Hot pages extraction complete\n");
    
    free(page_buffer);
    free(trackers);
    fclose(out);
    close(mem_fd);
    return 0;
}
