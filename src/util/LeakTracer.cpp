// (c) 1999 Erwin S. Andreasen <erwin@andreasen.org>
// Homepage: http://www.andreasen.org/LeakTracer/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int
    new_count, // how many memory blocks do we have
    leaks_count, // amount of entries in the below array
    first_free_spot; // Where is the first free spot in the leaks array?

static size_t new_size;  // total size

struct Leak {
    void *addr;
    size_t size;
    void *ret;
//    void *ret2; // Not necessary anymore
};
    
static Leak *leaks;

static void* register_alloc (size_t size) {
    new_count++;
    new_size += size;
    void *p = malloc(size);
    
    if (!p) { // We should really throw some sort of exception or call the new_handler
        fprintf(stderr, "LeakTracer: malloc: %m");
        _exit (1);
    }
    
    for (;;) {
        for (int i = first_free_spot; i < leaks_count; i++)
            if (leaks[i].addr == NULL) {
                leaks[i].addr = p;
                leaks[i].size = size;
                leaks[i].ret = __builtin_return_address(1);
//                leaks[i].ret2 = __builtin_return_address(2);
                first_free_spot = i+1;
                return p;
            }
        
        // Allocate a bigger array
        // Note that leaks_count starts out at 0.
        int new_leaks_count = leaks_count == 0 ? 16 : leaks_count * 2;
        leaks = (Leak*)realloc(leaks, sizeof(*leaks) * new_leaks_count);
        if (!leaks) {
            fprintf(stderr, "LeakTracer: realloc: %m");
            _exit(1);
        }
        memset(leaks+leaks_count, 0, sizeof(*leaks) * (new_leaks_count-leaks_count));
        leaks_count = new_leaks_count;
    }
}

static void register_free (void *p) {
    if (p == NULL)
        return;
    
    new_count--;
    for (int i = 0; i < leaks_count; i++)
        if (leaks[i].addr == p) {
            leaks[i].addr = NULL;
            new_size -= leaks[i].size;
            if (i < first_free_spot)
                first_free_spot = i;
            free(p);
            return;
        }

    fprintf(stderr, "LeakTracer: delete on an already deleted value?\n");
    abort();
}

void* operator new(size_t size) {
    return register_alloc(size);
}

void* operator new[] (size_t size) {
    return register_alloc(size);
}

void operator delete (void *p) {
    register_free(p);
}

void operator delete[] (void *p) {
    register_free(p);
}


// This ought to run at the very end, but it requires it to be put last on the linker line
void write_leaks() __attribute__((destructor));
void write_leaks() {
    const char *filename = getenv("LEAKTRACE_FILE") ? getenv("LEAKTRACE_FILE"): "leak.out";
    FILE *fp;
    
    if (!(fp = fopen(filename, "w")))
        fprintf(stderr, "LeakTracer: Could not open %s: %m\n", filename);
    else {
        for (int i = 0; i <  leaks_count; i++)
            if (leaks[i].addr != NULL) {
                // This ought to be 64-bit safe?
                fprintf(fp, "%8p %8p %9ld\n", leaks[i].addr, leaks[i].ret, (long) leaks[i].size);
            }
        
        fprintf(fp, "-------- %6d\n", new_size);
        fclose(fp);
    }
}
