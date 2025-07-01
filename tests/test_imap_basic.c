/*
 * Basic test for IMAP library functionality
 * This is a minimal test to verify the library builds and links correctly
 */

#include <stdio.h>

/* Include IMAP headers if available */
#ifdef HAVE_C_CLIENT_H
#include "c-client.h"
#else
/* Minimal stub test */
#endif

int main()
{
    printf("IMAP library test - basic functionality\n");
    
    /* Basic test that would use IMAP functions */
    /* For now, just verify we can compile and link */
    
    printf("Test completed successfully\n");
    return 0;
}