#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>

#define DEBUG

#define LOG_ERROR(fmt, args...)    fprintf(stderr, "ERROR: %s:%d:%s(): " fmt "\n", \
    __FILE__, __LINE__, __func__, ##args)

#ifdef DEBUG
#define LOG_DEBUG(fmt, args...)    fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", \
    __FILE__, __LINE__, __func__, ##args)
#else
//#define LOG_ERROR(fmt, args...)    /* Don't do anything in release builds */
#define LOG_DEBUG(fmt, args...)    /* Don't do anything in release builds */
#endif


#endif