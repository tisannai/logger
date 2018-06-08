/**
 * @file   logger.c
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Mon Jun  4 21:34:38 2018
 *
 * @brief  Logger - Logging facility.
 *
 */

#include <limits.h>

#include "logger.h"

void lg_void_assert( void );



/* ------------------------------------------------------------
 * Internal functions:
 */


static lg_grp_t lg_host_get_grp( lg_host_t host, const char* name )
{
    lg_grp_t grp;
    grp = mp_get_key( host->grps, (const gr_d)name );

    if ( grp )
        return grp;
    else
        lg_assert( 0 ); // GCOV_EXCL_LINE

    return sx_nil;
}


static lg_grp_t lg_host_check_grp( lg_host_t host, const char* name )
{
    return mp_get_key( host->grps, (const gr_d)name );
}


static void lg_host_add_grp( lg_host_t host, lg_grp_t grp )
{
    if ( lg_host_check_grp( host, grp->name ) == sx_nil ) {
        mp_put_key( host->grps, grp->name, grp );
    } else {
        lg_assert( 0 ); // GCOV_EXCL_LINE
    }
}


static lg_log_t lg_host_check_log( lg_host_t host, const char* name )
{
    return mp_get_key( host->logs, (const gr_d)name );
}


static void lg_host_add_log( lg_host_t host, lg_log_t log )
{
    mp_put_key( host->logs, log->name, log );
}


static lg_log_t lg_log_new( lg_log_type_t type, const char* name )
{
    lg_log_t log;

    log = gr_malloc( sizeof( lg_log_s ) );
    log->type = type;
    if ( name )
        log->name = strdup( name );
    log->fh = sx_nil;

    return log;
}


static void lg_log_del( lg_log_t log )
{
    if ( log->type == LG_LOG_TYPE_FILE && log->fh )
        fclose( log->fh );

    if ( log->name )
        gr_free( log->name );

    gr_free( log );
}


static lg_log_t lg_log_new_file( lg_host_t host, const char* name )
{
    lg_log_t log;
    lg_log_t file;

    if ( !strcmp( name, "<stdout>" ) ) {

        file = lg_host_check_log( host, name );
        if ( file == sx_nil ) {
            file = lg_log_new( LG_LOG_TYPE_STDOUT, name );
            file->fh = stdout;
            lg_host_add_log( host, file );
        }
        log = lg_log_new( LG_LOG_TYPE_LOGREF, file->name );
        log->log = file;

    } else {

        realpath( name, host->buf );
        sl_refresh( host->buf );

        file = lg_host_check_log( host, host->buf );
        if ( file == sx_nil ) {
            file = lg_log_new( LG_LOG_TYPE_FILE, host->buf );
            lg_host_add_log( host, file );
        }
        log = lg_log_new( LG_LOG_TYPE_LOGREF, file->name );
        log->log = file;
    }

    return log;
}


static void lg_log_write( lg_log_t log, const sl_t msg )
{
    if ( log->type == LG_LOG_TYPE_FILE ) {

        if ( log->fh == sx_nil ) {
            log->fh = fopen( log->name, "w" );
            if ( log->fh == sx_nil )
                lg_assert( 0 ); // GCOV_EXCL_LINE
        }

        fwrite( msg, 1, sl_length( msg ), log->fh );

    } else if ( log->type == LG_LOG_TYPE_STDOUT ) {

        fwrite( msg, 1, sl_length( msg ), log->fh );

    } else if ( log->type == LG_LOG_TYPE_LOGREF ) {

        lg_log_write( log->log, msg );

    } else if ( log->type == LG_LOG_TYPE_GRPREF ) {

        if ( log->grp->logs ) {
            lg_log_t ref;
            gr_each( log->grp->logs, ref, lg_log_t ) { lg_log_write( ref, msg ); }
        }

    } else {
        lg_assert( 0 ); // GCOV_EXCL_LINE
    }
}


static lg_grp_t lg_grp_new( lg_host_t host, lg_grp_type_t type, const char* name )
{
    lg_grp_t grp;

    grp = gr_malloc( sizeof( lg_grp_s ) );
    grp->type = type;
    grp->name = strdup( name );
    grp->prefix = sx_nil;
    grp->postfix = sx_nil;
    grp->logs = sx_nil;
    grp->active = sx_true;
    grp->top = sx_nil;
    grp->subs = sx_nil;

    lg_host_add_grp( host, grp );

    return grp;
}


static void lg_grp_del_logs( lg_grp_t grp )
{
    lg_log_t log;
    if ( grp->logs ) {
        gr_each( grp->logs, log, lg_log_t ) { lg_log_del( log ); }
        gr_reset( grp->logs );
    }
}


static void lg_grp_del( lg_grp_t grp )
{
    gr_free( grp->name );
    lg_grp_del_logs( grp );
    gr_destroy( &grp->logs );
    gr_destroy( &grp->subs );
}



static void lg_grp_enable( lg_grp_t grp )
{
    if ( grp->type == LG_GRP_TYPE_TOP ) {
        if ( grp->subs ) {
            lg_grp_t sub;
            gr_each( grp->subs, sub, lg_grp_t ) { lg_grp_enable( sub ); }
        }
    }

    grp->active = sx_true;
}


static void lg_grp_disable( lg_grp_t grp )
{
    if ( grp->type == LG_GRP_TYPE_TOP ) {
        if ( grp->subs ) {
            lg_grp_t sub;
            gr_each( grp->subs, sub, lg_grp_t ) { lg_grp_disable( sub ); }
        }
    }

    grp->active = sx_false;
}


static lg_grp_fn_p lg_grp_get_prefix( lg_grp_t grp )
{
    if ( grp->prefix ) {
        return grp->prefix;
    } else if ( grp->top ) {
        return lg_grp_get_prefix( grp->top );
    } else {
        return sx_nil;
    }
}


static lg_grp_fn_p lg_grp_get_postfix( lg_grp_t grp )
{
    if ( grp->postfix ) {
        return grp->postfix;
    } else if ( grp->top ) {
        return lg_grp_get_postfix( grp->top );
    } else {
        return sx_nil;
    }
}


static void lg_grp_write_msg( lg_host_t host, lg_grp_t grp, const sl_t msg )
{
    if ( grp->logs ) {
        lg_log_t log;
        gr_each( grp->logs, log, lg_log_t ) { lg_log_write( log, msg ); }
    }
}


static void lg_grp_write( lg_host_t   host,
                          lg_grp_t    grp,
                          int         newline,
                          const char* format,
                          va_list     ap )
{
    lg_grp_fn_p prefix = lg_grp_get_prefix( grp );
    lg_grp_fn_p postfix = lg_grp_get_postfix( grp );

    sl_clear( host->buf );

    if ( prefix )
        prefix( host, grp, format, &host->buf );

    sl_va_format( &host->buf, format, ap );

    if ( postfix )
        postfix( host, grp, format, &host->buf );

    if ( newline )
        sl_fill_with_char( &host->buf, '\n', 1 );

    lg_grp_write_msg( host, grp, host->buf );
}


static void lg_grp_join_grp_obj( lg_host_t host, lg_grp_t grp, lg_grp_t joinee )
{
    if ( grp && joinee ) {
        lg_log_t log;
        log = lg_log_new( LG_LOG_TYPE_GRPREF, NULL );
        log->grp = joinee;
        gr_add( &grp->logs, log );
    }
}


static void lg_grp_attach_sub( lg_host_t host, lg_grp_t top, lg_grp_t sub )
{
    gr_add( &top->subs, sub );
    sub->top = top;
    lg_grp_join_grp_obj( host, sub, top );
}




/* ------------------------------------------------------------
 * Callback functions:
 */

static void lg_host_grp_del_fn( gr_d key, gr_d value, void* arg )
{
    lg_grp_t grp = (lg_grp_t)value;
    lg_grp_del( grp );
}


static void lg_host_log_del_fn( gr_d key, gr_d value, void* arg )
{
    lg_log_t log = (lg_log_t)value;
    lg_log_del( log );
}




/* ------------------------------------------------------------
 * User API:
 */


lg_host_t lg_host_new( sx_t env )
{
    lg_host_t host;

    host = gr_malloc( sizeof( lg_host_s ) );
    host->env = env;
    host->grps = mp_new_full( mp_key_hash_cstr, mp_key_comp_cstr, 16, 50 );
    host->logs = mp_new_full( mp_key_hash_cstr, mp_key_comp_cstr, 16, 50 );
    host->buf = sl_new( PATH_MAX + 16 );

    host->quiet = sx_false;

    return host;
}


void lg_host_del( lg_host_t host )
{
    mp_each_key( host->grps, lg_host_grp_del_fn, sx_nil );
    mp_each_key( host->logs, lg_host_log_del_fn, sx_nil );
    mp_destroy( host->grps );
    mp_destroy( host->logs );
    sl_del( &host->buf );
    gr_free( host );
}


lg_grp_t lg_grp_top( lg_host_t   host,
                     const char* name,
                     const char* filename,
                     lg_grp_fn_p prefix,
                     lg_grp_fn_p postfix )
{
    lg_grp_t grp;

    grp = lg_grp_new( host, LG_GRP_TYPE_TOP, name );

    if ( filename )
        gr_add( &grp->logs, lg_log_new_file( host, filename ) );

    grp->prefix = prefix;
    grp->postfix = postfix;

    return grp;
}


lg_grp_t lg_grp_sub( lg_host_t host, const char* topname, const char* name )
{
    lg_grp_t grp;
    lg_grp_t top;

    top = lg_host_get_grp( host, topname );

    sl_clear( host->buf );
    sl_format_quick( &host->buf, "%s/%s", topname, name );

    grp = lg_grp_new( host, LG_GRP_TYPE_GRP, host->buf );
    lg_grp_attach_sub( host, top, grp );

    return grp;
}


lg_grp_t lg_grp_log( lg_host_t host, const char* name, const char* filename )
{
    lg_grp_t grp;

    grp = lg_grp_new( host, LG_GRP_TYPE_GRP, name );

    if ( filename )
        gr_add( &grp->logs, lg_log_new_file( host, filename ) );

    return grp;
}


void lg_grp_prefix( lg_host_t host, const char* name, lg_grp_fn_p prefix )
{
    lg_host_get_grp( host, name )->prefix = prefix;
}


void lg_grp_postfix( lg_host_t host, const char* name, lg_grp_fn_p postfix )
{
    lg_host_get_grp( host, name )->postfix = postfix;
}


void lg_grp_y( lg_host_t host, const char* name )
{
    lg_grp_enable( lg_host_get_grp( host, name ) );
}


void lg_grp_n( lg_host_t host, const char* name )
{
    lg_grp_disable( lg_host_get_grp( host, name ) );
}


void lg_grp_join_grp( lg_host_t host, const char* name, const char* joinee )
{
    lg_grp_t grp;
    lg_grp_t join_to;

    grp = lg_host_get_grp( host, name );
    join_to = lg_host_get_grp( host, joinee );

    if ( join_to->type == LG_GRP_TYPE_TOP )
        lg_grp_attach_sub( host, join_to, grp );
    else
        lg_grp_join_grp_obj( host, grp, join_to );
}


void lg_grp_join_file( lg_host_t host, const char* name, const char* joinee )
{
    lg_grp_t grp;

    grp = lg_host_get_grp( host, name );
    if ( joinee ) {
        gr_add( &grp->logs, lg_log_new_file( host, joinee ) );
    }
}


void lg_grp_merge_grp( lg_host_t host, const char* name, const char* joinee )
{
    lg_grp_t grp;
    lg_grp_t join_to;

    grp = lg_host_get_grp( host, name );
    join_to = lg_host_get_grp( host, joinee );

    if ( grp == join_to )
        lg_assert( 0 ); // GCOV_EXCL_LINE

    lg_grp_del_logs( grp );

    if ( join_to->type == LG_GRP_TYPE_TOP )
        lg_grp_attach_sub( host, join_to, grp );
    else
        lg_grp_join_grp_obj( host, grp, join_to );
}


void lg_grp_merge_file( lg_host_t host, const char* name, const char* joinee )
{
    lg_grp_t grp;

    grp = lg_host_get_grp( host, name );

    lg_grp_del_logs( grp );

    if ( joinee ) {
        gr_add( &grp->logs, lg_log_new_file( host, joinee ) );
    }
}


void lg( lg_host_t host, const char* name, const char* format, ... )
{
    va_list ap;

    lg_grp_t grp;
    grp = lg_host_get_grp( host, name );

    if ( grp->active ) {
        va_start( ap, format );
        lg_grp_write( host, grp, 1, format, ap );
        va_end( ap );
    }
}


void lgw( lg_host_t host, const char* name, const char* format, ... )
{
    va_list ap;

    lg_grp_t grp;
    grp = lg_host_get_grp( host, name );

    if ( grp->active ) {
        va_start( ap, format );
        lg_grp_write( host, grp, 0, format, ap );
        va_end( ap );
    }
}






/**
 * Disabled (void) assertion.
 */
void lg_void_assert( void ) // GCOV_EXCL_LINE
{
}
