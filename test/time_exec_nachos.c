
int main(){
    int i, pid;
    for( i = 0; i < 10; i += 2 ){
        i--;
        Join( Exec( "test/time_exec_in_nachos" ) );
    }
}
