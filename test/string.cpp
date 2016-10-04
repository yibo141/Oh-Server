#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

ssize_t read_to(int fd, char *buffer)
{
    size_t bytes_read = 0;
    size_t bytes_left = 500;
    char *ptr = buffer;
    while(bytes_left > 0)
    {
        bytes_read = read(fd, ptr, bytes_left);
        if(bytes_read == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                bytes_read = 0;
            else 
                return -1;
        }
        else if(bytes_read == 0)
            break;
        bytes_left -= bytes_read;
        ptr += bytes_read;
    }
    return 500 - bytes_left;
}


int main()
{
    char buf[500];
    int fd = open("./text", O_RDONLY, 0);
    // read_to(fd, buf);
    // cout << string(buf) << endl;
    int n = read(fd, buf, 500);
    cout << n << endl;
    cout << string(buf).size() << endl;
    return 0;
}
