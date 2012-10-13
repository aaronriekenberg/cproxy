#ifndef ERRUTIL_H
#define ERRUTIL_H

/* Similar to strerror, but:
 * Thread safe.
 * Returns a char array allocated by malloc which the caller must free. */
extern char* errnoToString(int errnoToTranslate);

#endif
