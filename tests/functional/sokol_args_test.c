//------------------------------------------------------------------------------
//  sokol-args-test.c
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#include "sokol_args.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)
#define TSTR(s0, s1) EXPECT_TRUE(0 == strcmp(s0,s1))
#define NUM_ARGS(x) (sizeof(x)/sizeof(void*))

static char* argv_0[] = { "exe_name " };
UTEST(sokol_args, init_shutdown) {
    sargs_setup(&(sargs_desc){0});
    T(sargs_isvalid());
    T(_sargs.max_args == _SARGS_MAX_ARGS_DEF);
    T(_sargs.args);
    T(_sargs.buf_size == _SARGS_BUF_SIZE_DEF);
    T(_sargs.buf_pos == 1);
    T(_sargs.buf);
    T(sargs_num_args() == 0);
    TSTR(sargs_key_at(0), "");
    TSTR(sargs_value_at(0), "");
    sargs_shutdown();
    T(!sargs_isvalid());
    T(0 == _sargs.args);
    T(0 == _sargs.buf);
}

UTEST(sokol_args, no_args) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_0),
        .argv = argv_0
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 0);
    TSTR(sargs_key_at(0), "");
    TSTR(sargs_value_at(0), "");
    T(-1 == sargs_find("bla"));
    T(!sargs_exists("bla"));
    TSTR(sargs_value("bla"), "");
    TSTR(sargs_value_def("bla", "blub"), "blub");
    sargs_shutdown();
}

static char* argv_1[] = { "exe_name", "kvp0=val0", "kvp1=val1", "kvp2=val2" };
UTEST(sokol_args, simple_args) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_1),
        .argv = argv_1,
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "val0");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "val0");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), "val1");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), "val1");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "val2");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "val2");
    T(_sargs.buf_pos == 31);
    sargs_shutdown();
}

static char* argv_2[] = { "exe_name", "kvp0  = val0 ", "  \tkvp1=  val1", "kvp2  = val2   "};
UTEST(sokol_args, simple_whitespace) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_2),
        .argv = argv_2
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "val0");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "val0");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), "val1");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), "val1");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "val2");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "val2");
    T(_sargs.buf_pos == 31);
    sargs_shutdown();
}

static char* argv_4[] = { "exe_name", "kvp0 ", "=val0 ", "  kvp1", "=", "val1", "kvp2 \t", "= val2   "};
UTEST(sokol_args, standalone_separator) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_4),
        .argv = argv_4
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "val0");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "val0");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), "val1");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), "val1");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "val2");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "val2");
    T(_sargs.buf_pos == 31);
    sargs_shutdown();
}

static char* argv_5[] = { "exe_name", "kvp0='bla bla'", "kvp1=' blub blub'", "kvp2='blob blob '"};
UTEST(sokol_args, single_quotes) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_5),
        .argv = argv_5
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "bla bla");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "bla bla");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), " blub blub");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), " blub blub");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "blob blob ");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "blob blob ");
    sargs_shutdown();
}

static char* argv_6[] = { "exe_name", "kvp0=\"bla bla\"", "kvp1=\" blub blub\"", "kvp2=\"blob blob \""};
UTEST(sokol_args, double_quotes) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_6),
        .argv = argv_6
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "bla bla");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "bla bla");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), " blub blub");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), " blub blub");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "blob blob ");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "blob blob ");
    sargs_shutdown();
}

static char* argv_7[] = { "exe_name", "kvp0='bla \"bla\"'", "kvp1=' \"blub blub\"'", "kvp2='blob \"blob\" '"};
UTEST(sokol_args, double_in_single_quotes) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_7),
        .argv = argv_7
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "bla \"bla\"");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "bla \"bla\"");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), " \"blub blub\"");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), " \"blub blub\"");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "blob \"blob\" ");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "blob \"blob\" ");
    sargs_shutdown();
}

static char* argv_8[] = { "exe_name", "kvp0=\"bla 'bla'\"", "kvp1=\" 'blub blub'\"", "kvp2=\"blob 'blob' \""};
UTEST(sokol_args, single_in_double_quotes) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_8),
        .argv = argv_8
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "bla 'bla'");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "bla 'bla'");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), " 'blub blub'");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), " 'blub blub'");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "blob 'blob' ");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "blob 'blob' ");
    sargs_shutdown();
}

static char* argv_9[] = { "exe_name", "kvp0='bla ", "bla'", "kvp1= ' blub", " blub'", "kvp2='blob blob '"};
UTEST(sokol_args, test_split_quotes) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_9),
        .argv = argv_9
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "bla bla");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "bla bla");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), " blub blub");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), " blub blub");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "blob blob ");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "blob blob ");
    sargs_shutdown();
}

static char* argv_10[] = { "exe_name", "kvp0=\\\\val0\\nval1", "kvp1=val1\\rval2", "kvp2='val2\\tval3'" };
UTEST(sokol_args, escape_sequence) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_10),
        .argv = argv_10,
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 3);
    T(0 == sargs_find("kvp0"));
    TSTR(sargs_value("kvp0"), "\\val0\nval1");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_value_at(0), "\\val0\nval1");
    T(1 == sargs_find("kvp1"));
    TSTR(sargs_value("kvp1"), "val1\rval2");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_value_at(1), "val1\rval2");
    T(2 == sargs_find("kvp2"));
    TSTR(sargs_value("kvp2"), "val2\tval3");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_value_at(2), "val2\tval3");
    sargs_shutdown();
}

static char* argv_11[] = { "exe_name", "kvp0 kvp1", "kvp2 = val2", "kvp3", "kvp4=val4" };
UTEST(sokol_args, key_only_args) {
    sargs_setup(&(sargs_desc){
        .argc = NUM_ARGS(argv_11),
        .argv = argv_11,
    });
    T(sargs_isvalid());
    T(sargs_num_args() == 5);
    T(0 == sargs_find("kvp0"));
    T(1 == sargs_find("kvp1"));
    T(2 == sargs_find("kvp2"));
    T(3 == sargs_find("kvp3"));
    T(4 == sargs_find("kvp4"))
    T(-1 == sargs_find("kvp5"));
    T(-1 == sargs_find("val2"));
    T(-1 == sargs_find("val4"));
    T(sargs_exists("kvp0"));
    T(sargs_exists("kvp1"));
    T(sargs_exists("kvp2"));
    T(sargs_exists("kvp3"));
    T(sargs_exists("kvp4"));
    T(!sargs_exists("kvp5"));
    TSTR(sargs_value("kvp0"), "");
    TSTR(sargs_value("kvp1"), "");
    TSTR(sargs_value("kvp2"), "val2");
    TSTR(sargs_value("kvp3"), "");
    TSTR(sargs_value("kvp4"), "val4");
    TSTR(sargs_value("kvp5"), "");
    TSTR(sargs_value_def("kvp0", "bla0"), "bla0");
    TSTR(sargs_value_def("kvp1", "bla1"), "bla1");
    TSTR(sargs_value_def("kvp2", "bla2"), "val2");
    TSTR(sargs_value_def("kvp3", "bla3"), "bla3");
    TSTR(sargs_value_def("kvp4", "bla4"), "val4");
    TSTR(sargs_value_def("kvp5", "bla5"), "bla5");
    TSTR(sargs_key_at(0), "kvp0");
    TSTR(sargs_key_at(1), "kvp1");
    TSTR(sargs_key_at(2), "kvp2");
    TSTR(sargs_key_at(3), "kvp3");
    TSTR(sargs_key_at(4), "kvp4");
    TSTR(sargs_key_at(5), "");
    TSTR(sargs_value_at(0), "");
    TSTR(sargs_value_at(1), "");
    TSTR(sargs_value_at(2), "val2");
    TSTR(sargs_value_at(3), "");
    TSTR(sargs_value_at(4), "val4");
    TSTR(sargs_value_at(5), "");
}
