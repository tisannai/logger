#ifndef LOGGER_H
#define LOGGER_H

/**
 * @file   logger.h
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Mon Jun  4 21:34:38 2018
 *
 * @brief  Logger - Logging facility.
 *
 */



#include <stdlib.h>
#include <stdint.h>
#include <sixten.h>
#include <gromer.h>
#include <mapper.h>
#include <slinky.h>


#ifndef LOGGER_NO_ASSERT
#include <assert.h>
/** Default assert. */
#define lg_assert assert
#else
/** Disabled assertion. */
#define lg_assert( cond ) (void)( ( cond ) || lg_void_assert() )
#endif


sx_struct( lg_host )
{
    sx_t      data;        /**< User data. */
    mp_t      grps;        /**< Logger Groups. */
    mp_t      logs;        /**< Logger Logs. */
    sx_bool_t disabled;    /**< Silence Host. */
    sl_t      buf;         /**< String building buffer. */
    sx_bool_t conf_active; /**< Config: active. */
};


sx_struct_type( lg_grp );
sx_struct_type( lg_log );

sx_enum( lg_log_type ){ LG_LOG_TYPE_NONE = 0,
                        LG_LOG_TYPE_FILE,
                        LG_LOG_TYPE_STDOUT,
                        LG_LOG_TYPE_GRPREF,
                        LG_LOG_TYPE_LOGREF };

sx_struct( lg_log )
{
    lg_log_type_t type; /**< Log type. */
    char*         name; /**< Log file name ("<stdout>" for STDOUT). */
    union
    {
        FILE*    fh;  /**< File handle (LG_LOG_TYPE_FILE/LG_LOG_TYPE_STDOUT). */
        lg_grp_t grp; /**< Grp reference (LG_LOG_TYPE_GRPREF). */
        lg_log_t log; /**< Grp reference (LG_LOG_TYPE_LOGREF). */
    };
};



/**
 * Log Group callback (prefix/postfix).
 *
 * "msg" is a reference to the log message. "output" is storage for
 * callback result.
 */
typedef void ( *lg_grp_fn_p )( const lg_host_t host,
                               const lg_grp_t  grp,
                               const char*     msg,
                               sl_p            outbuf );


sx_enum( lg_grp_type ){ LG_GRP_TYPE_NONE = 0, LG_GRP_TYPE_TOP, LG_GRP_TYPE_GRP };

sx_struct( lg_grp )
{
    lg_grp_type_t type;    /**< Type. */
    char*         name;    /**< Name. */
    lg_grp_fn_p   prefix;  /**< Prefix function. */
    lg_grp_fn_p   postfix; /**< Postfix function. */
    gr_t          logs;    /**< List of Logs. */
    sx_bool_t     active;  /**< Grp is active? */
    lg_grp_t      top;     /**< Grp top (if any). */
    gr_t          subs;    /**< List of Subs (if any). */
};


/**
 * Create Log Host.
 *
 * @param data User data.
 *
 * @return Host.
 */
lg_host_t lg_host_new( sx_t data );


/**
 * Destroy Log Host.
 *
 * @param host Host.
 */
void lg_host_del( lg_host_t host );


/**
 * Return user data.
 *
 * @param host Host.
 *
 * @return User data.
 */
sx_t lg_host_data( lg_host_t host );


/**
 * Enable Host output.
 *
 * @param host Host.
 */
void lg_host_y( lg_host_t host );


/**
 * Disable Host output.
 *
 * @param host Host.
 */
void lg_host_n( lg_host_t host );


/**
 * Configure Host defaults.
 *
 * Configs: "active".
 *
 * @param host   Host.
 * @param config Config name.
 * @param value  Config value.
 */
void lg_host_config( lg_host_t host, const char* config, sx_bool_t value );


/**
 * Create Top Group.
 *
 * Top is used to control other groups, but is can also be used as
 * Primary Group.
 *
 * @param host      Host.
 * @param name      Group name.
 * @param filename  Group file (or NULL).
 * @param prefix    Prefix function (or NULL).
 * @param postfix   Postfix function (or NULL).
 *
 * @return Group.
 */
lg_grp_t lg_grp_top( lg_host_t   host,
                     const char* name,
                     const char* filename,
                     lg_grp_fn_p prefix,
                     lg_grp_fn_p postfix );


/**
 * Create Sub Group for Top.
 *
 * @param host    Host.
 * @param topname Name of Top Group.
 * @param name    Name of Sub Group.
 *
 * @return Group.
 */
lg_grp_t lg_grp_sub( lg_host_t host, const char* topname, const char* name );


/**
 * Create Group.
 *
 * Group is not part of any Top Group. If "filename" is not given, the
 * file can be attached later with lg_grp_join_file().
 *
 * @param host     Host.
 * @param name     Group name.
 * @param filename Group file (or NULL).
 *
 * @return Group.
 */
lg_grp_t lg_grp_log( lg_host_t host, const char* name, const char* filename );


/**
 * Assign Prefix Function to Group.
 *
 * @param host   Host.
 * @param name   Group name.
 * @param prefix Prefix function.
 */
void lg_grp_prefix( lg_host_t host, const char* name, lg_grp_fn_p prefix );


/**
 * Assign Postfix Function to Group.
 *
 * @param host   Host.
 * @param name   Group name.
 * @param postfix Postfix function.
 */
void lg_grp_postfix( lg_host_t host, const char* name, lg_grp_fn_p postfix );


/**
 * Enable Group logging (Yes).
 *
 * @param host Host.
 * @param name Group name.
 */
void lg_grp_y( lg_host_t host, const char* name );


/**
 * Disable Group logging (No).
 *
 * @param host Host.
 * @param name Group name.
 */
void lg_grp_n( lg_host_t host, const char* name );


/**
 * Join Group to logging of another Group.
 *
 * @param host   Host.
 * @param name   Group name of joiner.
 * @param joinee Group name of joinee.
 */
void lg_grp_join_grp( lg_host_t host, const char* name, const char* joinee );


/**
 * Join Group to logging File.
 *
 * @param host   Host.
 * @param name   Group name of joiner.
 * @param joinee File name of joinee.
 */
void lg_grp_join_file( lg_host_t host, const char* name, const char* joinee );


/**
 * Merge Group to another Group.
 *
 * @param host   Host.
 * @param name   Group name of joiner.
 * @param joinee Group name of joinee.
 */
void lg_grp_merge_grp( lg_host_t host, const char* name, const char* joinee );


/**
 * Merge Group to logging File.
 *
 * @param host   Host.
 * @param name   Group name of joiner.
 * @param joinee File name of joinee.
 */
void lg_grp_merge_file( lg_host_t host, const char* name, const char* joinee );


/**
 * Attach Group to Top.
 *
 * @param host Host.
 * @param top  Top.
 * @param name Group name.
 */
void lg_grp_attach( lg_host_t host, const char* top, const char* name );


/**
 * Detach Group from Top.
 *
 * @param host Host.
 * @param top  Top.
 * @param name Group name.
 */
void lg_grp_detach( lg_host_t host, const char* top, const char* name );


/**
 * Log message with newline.
 *
 * @param host   Host.
 * @param name   Group name.
 * @param format Message formatter.
 */
void lg( lg_host_t host, const char* name, const char* format, ... );


/**
 * Log message without newline.
 *
 * @param host   Host.
 * @param name   Group name.
 * @param format Message formatter.
 */
void lgw( lg_host_t host, const char* name, const char* format, ... );


/**
 * Inactive assertion.
 *
 */
void lg_void_assert( void );


#endif
