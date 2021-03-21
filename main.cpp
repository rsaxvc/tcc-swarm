#include <vector>
#include <cstdio>
#include <libtcc.h>

static int compile_unit( const char * input, const char * output, std::vector<const char*> & inc )
{
    TCCState* s = tcc_new();
    if(!s){
        printf("Can’t create a TCC context\n");
        return 1;
    }
    tcc_set_output_type(s, TCC_OUTPUT_OBJ);

    if( tcc_add_file(s, input) <0) {
        printf("error adding file\n");
        return 2;
    }

    if( tcc_output_file(s, output) < 0 ) {
        printf("outputfile error\n");
        return 3;
    }

    tcc_delete(s);
    return 0;
}



int main(int argc, char **argv)
{
    std::vector<const char *> includes;
    compile_unit("test/main.c","test/main.o", includes);
    compile_unit("test/funcA.c","test/funcA.o", includes);
    compile_unit("test/funcB.c","test/funcB.o", includes);
    compile_unit("test/funcC.c","test/funcC.o", includes);
    return 0;
}

