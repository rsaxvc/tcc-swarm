#include <stdio.h>
#include <libtcc.h>

const char* program = \
    "int foo(const int in_value){"
    "   printf(\"this is a test: %d\n\", in_value);"
    "}";


int main(int argc, char **argv)
{
    TCCState* s = tcc_new();
    if(!s){
        printf("Can’t create a TCC context\n");
        return 1;
    }
    tcc_set_output_type(s, TCC_OUTPUT_OBJ);

    if (tcc_compile_string(s, program) > 0) {
        printf("Compilation error !\n");
        return 2;
    }

    if( tcc_output_file(s, "derp.o") > 0 ) {
        printf("outputfile error\n");
        return 3;
    }


    tcc_relocate(s, TCC_RELOCATE_AUTO);

    int (*const foo)(const int in_value) = tcc_get_symbol(s, "foo");
    foo(32);

    tcc_delete(s);
    return 0;
}

