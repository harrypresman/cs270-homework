#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(){
    char* file = "/home/aelmore/temp/tempc1";
    open(file,O_RDONLY);
}