//
// Created by Abhishikth Nandam on 2/11/17.
//

#include "utility.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void parse_GET(char *buf) {
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
}