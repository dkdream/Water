
#include "compiler.h"
#include <copper.h>

/* */
#define _GNU_SOURCE
#include <libgen.h>
#include <getopt.h>
#include <stdbool.h>

/* */
unsigned int h2o_global_debug = 0;

/* */
static char*  program_name = 0;

static void usage(char *name)
{
    fprintf(stderr, "usage: water  [--verbose]+ --name c_func_name [--output outfile] [--file infile]\n");
    fprintf(stderr, "water [-v]+ -n c_func_name [-o outfile] [-f infile]\n");
    fprintf(stderr, "water [--help]\n");
    fprintf(stderr, "water [-h]\n");
    fprintf(stderr, "where <option> can be\n");
    fprintf(stderr, "  -h|--help    print this help information\n");
    fprintf(stderr, "  -v|--verbose be verbose\n");
    fprintf(stderr, "if no <infile> is given, input is read from stdin\n");
    fprintf(stderr, "if no <oufile> is given, output is written to stdou\n");
    exit(1);
}

int main(int argc, char **argv)
{
    program_name = basename(argv[0]);

    const char* infile   = 0;
    const char* outfile  = 0;
    const char* funcname = 0;

    unsigned do_trace  = 0;
    unsigned do_debug  = 0;

    static struct option long_options[] = {
        {"trace",   0, 0, 't'},
        {"verbose", 0, 0, 'v'},
        {"help",    0, 0, 'h'},
        {"file",    1, 0, 'f'},
        {"output",  1, 0, 'o'},
        {"name",    1, 0, 'n'},
        {0, 0, 0, 0}
    };

    int chr;
    int option_index = 0;

    while (-1 != ( chr = getopt_long(argc, argv,
                                     "vthf:n:o:",
                                     long_options,
                                     &option_index)))
        {
            switch (chr)
                {
                case 'h':
                    usage(argv[0]);
                    break;

                case 'v':
                    ++do_debug;
                    break;

                case 't':
                    ++do_trace;
                    break;

                case 'f':
                    infile = optarg;
                    break;

                case 'o':
                    outfile = optarg;
                    break;

                case 'n':
                    funcname = optarg;
                    break;

                default:
                    printf("invalid option %c\n", chr);
                    exit(1);
                }
        }

    argc -= optind;
    argv += optind;

    cu_global_debug  = do_trace;
    h2o_global_debug = do_debug;

    if (!funcname) {
        fprintf(stderr, "a function name MUST be defined\n");
        exit(1);
    }


    H2oParser water = 0;

    if (!water_Create(infile, outfile, &water)) {
        fprintf(stderr, "unable to create parser\n");
        exit(1);
    }

    water_Parse(water, funcname);

    if (!water_Free(water)) {
        fprintf(stderr, "unable to free parser\n");
        exit(1);
    }

    return 0;
}
