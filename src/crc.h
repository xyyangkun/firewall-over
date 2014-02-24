/*
 * crc.h
 *
 *  Created on: 2014-2-23
 *      Author: xy
 */

#ifndef CRC_H_
#define CRC_H_
#include <iostream>
#include <cstdio>
using namespace std;

int myprint(const unsigned char  *p, long size);
unsigned int  crc32( unsigned char *buf, unsigned int size);

#endif /* CRC_H_ */
