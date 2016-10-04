#include <iostream>
#include "http_process.h"
#include "http_parser.h"

using namespace std;

int main()
{
    char buf[500];
    int fd = open("./text", O_RDONLY, 0);
    // int n = read(fd, buf, 100);
    // cout << string(buf) << endl;
    // cout << "-----------------------------" << endl;
    // write(STDOUT_FILENO, buf, n);
    http_process hp(fd);
    hp.process();
    return 0;
}
