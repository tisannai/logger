#include "unity.h"
#include "logger.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>



/* ------------------------------------------------------------
 * Helper functions:
 */

void prefix( const lg_host_t host, const lg_grp_t grp, const char* msg, sl_p outbuf )
{
    (void)host;
    (void)grp;
    (void)msg;

    sl_concatenate_c( outbuf, "prefix: " );
}


void empty_prefix( const lg_host_t host, const lg_grp_t grp, const char* msg, sl_p outbuf )
{
    (void)host;
    (void)grp;
    (void)msg;
    (void)outbuf;
}

void postfix( const lg_host_t host, const lg_grp_t grp, const char* msg, sl_p outbuf )
{
    (void)host;
    (void)grp;
    (void)msg;
    sl_concatenate_c( outbuf, "\n" );
}


void check_file_content( const char* file, const char* content )
{
    sl_t ss;
    ss = sl_read_file( file );
    TEST_ASSERT_TRUE( !strcmp( ss, content ) );
    sl_del( &ss );
}


int check_file_exists( const char* file )
{
    FILE* fh;
    fh = fopen( file, "r" );
    return ( fh != NULL );
}


void prepare_testout( void )
{
    system( "mkdir -p test/out" );
}


void clean_testout( void )
{
    system( "rm -rf test/out" );
}



/* ------------------------------------------------------------
 * Tests:
 */

void test_basic( void )
{
    lg_host_t host;
    char*     message = "message";

    prepare_testout();

    host = lg_host_new( st_nil );
    TEST_ASSERT_TRUE( host != st_nil );
    lg_host_config( host, "active", st_true );

    lg_grp_log( host, "simple", "test/out/simple.log" );
    lg( host, "simple", "my message is this: %s, %s", message, message );

    lg_host_del( host );

    check_file_content( "test/out/simple.log", "my message is this: message, message\n" );

    clean_testout();
}


void test_top( void )
{
    lg_host_t host;
    char*     message = "message";

    prepare_testout();

    host = lg_host_new( message );
    TEST_ASSERT_TRUE( host != st_nil );
    TEST_ASSERT_TRUE( !strcmp ( message, ((char*)lg_host_data( host ) ) ) );

    lg_grp_top( host, "simple", "test/out/simple.log", prefix, st_nil );
    lg( host, "simple", "for message: %s", message );
    lg_grp_sub( host, "simple", "sub" );
    lg( host, "simple/sub", "sub message: %s", message );

    lg_grp_top( host, "top", "test/out/top.log", st_nil, postfix );
    lgw( host, "top", "1" );
    lg_grp_sub( host, "top", "sub" );
    lgw( host, "top/sub", "2" );
    lg_grp_prefix( host, "top", empty_prefix );
    lg_grp_postfix( host, "top", st_nil );
    lgw( host, "top", "3" );
    lgw( host, "top/sub", "4" );
    lg_grp_n( host, "top" );
    lgw( host, "top", "5" );
    lgw( host, "top/sub", "6" );
    lg_grp_y( host, "top/sub" );
    lgw( host, "top", "7" );
    lgw( host, "top/sub", "8" );
    lg_grp_y( host, "top" );
    lgw( host, "top", "9" );
    lgw( host, "top/sub", "0" );


    lg_host_del( host );

    check_file_content( "test/out/simple.log",
                        "prefix: for message: message\nprefix: sub message: message\n" );
    check_file_content( "test/out/top.log", "1\n2\n34890" );

    clean_testout();
}


void test_files( void )
{
    lg_host_t host;

    prepare_testout();

    host = lg_host_new( st_nil );
    TEST_ASSERT_TRUE( host != st_nil );

    lg_grp_top( host, "no_writes", "test/out/no_writes.log", st_nil, st_nil );
    TEST_ASSERT_TRUE( check_file_exists( "test/out/no_writes.log" ) == 0 );

    lg_grp_top( host, "do_writes", st_nil, st_nil, st_nil );
    lg_grp_sub( host, "do_writes", "sub1" );
    lg_grp_log( host, "do_writes/sub3", st_nil );
    lg_grp_join_grp( host, "do_writes/sub3", "do_writes" );
    lg_grp_join_file( host, "do_writes/sub1", "test/out/do_writes.log" );

    lg_grp_log( host, "do_writes/sub2", st_nil );
    lg_grp_join_grp( host, "do_writes/sub2", "do_writes/sub1" );
    lg_grp_join_file( host, "do_writes/sub2", "test/out/do_writes2.log" );

    lg_grp_log( host, "do_writes/all", "test/out/all_writes.log" );

    lgw( host, "do_writes/sub2", "1" );
    lgw( host, "do_writes/sub1", "2" );

    lg_grp_merge_file( host, "do_writes/sub1", "test/out/all_writes.log" );
    lg_grp_merge_grp( host, "do_writes/sub2", "do_writes/sub1" );
    lg_grp_merge_grp( host, "do_writes", "do_writes/sub1" );
    lg_grp_merge_grp( host, "do_writes/sub3", "do_writes" );
    lg_grp_attach( host, "do_writes/sub3", "do_writes" );
    lg_grp_detach( host, "do_writes/sub3", "do_writes" );

    lgw( host, "do_writes/sub1", "3" );
    lgw( host, "do_writes/sub2", "4\n" );
    lgw( host, "do_writes/sub3", "5" );

    lg_grp_log( host, "stdout", "<stdout>" );
    lgw( host, "stdout", "" );

    lg_host_y( host );
    lg_host_n( host );
    lgw( host, "do_writes/sub1", "3" );
    lgw( host, "do_writes/sub2", "4\n" );
    lgw( host, "do_writes/sub3", "5" );

    lg_host_del( host );

    check_file_content( "test/out/all_writes.log", "34\n5" );
    check_file_content( "test/out/do_writes.log", "12" );
    check_file_content( "test/out/do_writes2.log", "1" );

    clean_testout();
}
