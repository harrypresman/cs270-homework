/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

void testFunc(){
    int x = 5;
    x += 5;
    x -= 2;
    Exit( 0 );
}

int main(){
    int pid, fd1, fd2, fd3;
    char* buf = "banana";
    int size = 7;

    // test fork, exec, join, yield
/*
    Fork( testFunc );
    Exec( "test/halt2" );
    pid = Exec( "test/halt2" );
    Join( pid );
    Yield();
*/
    // test create, open, write, read, close
    Create( "apple" );
    Create( "orange" );
    Create( "mango" );
    Create( "berry" );

    fd1 = Open( "apple" );
    fd2 = Open( "orange" );
    fd3 = Open( "berry" );
    Write( buf, size, fd1 );
    Read( buf, size, fd1 );
    Write( buf, size, fd2 );
    Write( buf, size, fd3 );
    Close( "apple" );
    Close( "berry" );
    Close( "orange" );
    fd1 = Open( "mango" );
    Write( buf, size, fd1 );
    Close( "mango" );

    Halt();
    /* not reached */
}
