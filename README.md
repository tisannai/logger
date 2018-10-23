# Logger - Flexible logging

Logger library is a general purpose message logger. The library is
build on top of three basic concepts:

* Host - library host contains all Groups and Files.

* Group - has a name as identifier, associated Logs, and
  active/inactive state.

* Log - is reference to a Log Entry. Log Entry can be a File Reference
  (also STDOUT), or Group Reference.

Group has two types: `top`, and `normal`. Top Group can be used to
propagate properties to its Sub Groups. Sub Groups are created by
associating them a Top.

## Introduction

Let's examine few examples. First we create a Host:

    lg_host_t host;
    host = lg_host_new( appl_info );

`lg_host_new` creates the Log Host and initializes it with
`appl_info`. Application information (user data) can be whatever and
it will be only passed forward to user defined functions (as we'll see
later).

In order to produce log message, we create a Log Group:

    lg_grp_log( host, "run", "run.log" );

`lg_grp_log` creates a Log Group with associated Log that references
the File `run.log`. Note that "run" is completely separate name from
"run.log", but it makes to sense to maintain consistent naming betwee
Groups and Files, if possible. `lg_grp_log` returns a reference to the
created Group, but it is not typically needed for anything, since
Groups are referenced by name.

"run.log" disk file is opened only after the message have been written
to the Log. This prevents creating zero sized files for unused Log
Files. Groups are active at creation by default, however Host can be
configured to create disabled Groups by default (`gs_host_config`).

We can write log messages, by referring to the Group name:

    lg( host, "run", "Log message to run.log from: %s", __func__ );

`lg` function writes the formatted message to a File defined by Group
"run", which is hosted by `host`. "run" is used in efficient hash
lookup to find the actual Group Object related to "run". User does not
have to worry about the details, it is enough to know the Group name
and maintain a handle to the Host.

Before program exists, the Host should be destroyed. This will ensure
that all files are flushed and all memory is released.

    lg_host_del( host );

In a lot of simple programs the features above are all that is
needed. However, there are cases where more control is needed, and
thus Logger provides more control over hosted Groups.


## Group Hierarchy

It is a fairly common use case to use a shared Log File for multiple
groups, and change the active collection of Groups from one program
run to another. The simplest way to facilitate this is to first create
a Top Group for all other Groups.

    lg_grp_top( host, "log", "exec.log" );

Then create the rest of the groups as Sub Groups for the top.

     lg_grp_sub( host, "log", "debug" );
     lg_grp_sub( host, "log", "report" );

Two Sub Groups are created: "log/debug", and "log/report". The Sub
Group names are thus created by concatenating the given Top name (2nd
argument) and passed name argument (3rd argument).

Top is a control point for the Sub Groups. For example if we disable
Top, we indirectly disable also the decendants.

    lg_grp_n( host, "log" );

The same can be done in enabling as well.

    lg_grp_y( host, "log" );

Individual Sub Groups can be controlled by calling enable for the
distinct Sub Group.

    lg_grp_y( host, "log/debug" );

Setting enable or disable from Top overwrites the enable settings for
decendants. Thus deviations to common setting should be set after
setting Top.

Group can be explicitly assigned to a Top.

    lg_grp_attach( host, "some/group", "log" );

"some/group" becomes Sub Group of "log" and hence behaves as a
decendant. The opposite operation is:

    lg_grp_detach( host, "some/group", "log" );

This will remove Sub Group from Top, and hence control of Top will not
effect this, former, Sub Group any more. Group may have only one Top.

Top Group is allowed to be a top of another Top Group. Hence a full
hierarchy might be constructed. This might be useful if there are
different concern depending on the hierarchy levels. For example
higher level is concerned about enabling and lower layers about Prefix
and Postfix (see below) arrangements.

Host can be disabled as well (`lg_host_n`), which means that all
Groups becomes silent.


## Prefix and Postfix

Another common use case for the Top paradigm, is to have a common
Prefix (and/or Postfix) to be output along with the main log message
by all decendants. Prefix is performed before message and Postfix
after the message. They are otherwise the same, and thus only Prefix
is explained in detail from now on.

Prefix is a callback function that user defines according to required
function signature.

    typedef void ( *lg_grp_fn_p )( const lg_host_t host,
                                   const lg_grp_t  grp,
                                   const char*     msg,
                                   sl_p            outbuf );

Prefix callback gets `host`, `grp`, and prepared `msg` as input, and
it stores its result to `outbuf`. Prefix function has the possiblity
to render the prefix based on all the input information. Note that
"user data" is captured inside Host, and can accessed with
`lg_host_data`. `output` is a Slinky which grows according to the
stored output.


## Joining and Merging

A possible use case is to output the same message to multiple
Files. For example one might want to have separate files for all
groups, but also a concatenated collection of all message in the
correct relative order between groups.

Joining is used for "branching" a Group to multiple Logs. Joining can
be done to distinct Log File or to another Group. If target is a
Group, all the Group's current Logs will be joined. Note that joining
creates a reference and does not capture a specific set of
Logs. Instead it is dynamic reference, and only the currently listed
Logs of the referred Group are in action.

    lg_grp_join_grp( host, "some/group", "log" );
    lg_grp_join_file( host, "some/group", "run.log" );

In the first line "some/group" is joining Group called "log". In the
second line "some/group" is joining Log File "run.log".

Merging is the opposite of Joining. In merging the output is reduced
to selected target. Again it might be Log File or another Group. Log
association can be removed completely by providing `NULL` for the
name.

    lg_grp_merge_grp( host, "some/group", "log" );
    lg_grp_merge_file( host, "some/group", "run.log" );



## More details

See Doxygen docs and `logger.h` for details about Logger API. Also
consult the test directory for usage examples.



## TODO

* Wildcards for referencing multiple groups with one string.




## Logger API documentation

See Doxygen documentation. Documentation can be created with:

    shell> doxygen .doxygen


## Examples

All functions and their use is visible in tests. Please refer `test`
directory for testcases.


## Building

Ceedling based flow is in use:

    shell> ceedling

Testing:

    shell> ceedling test:all

User defines can be placed into `project.yml`. Please refer to
Ceedling documentation for details.


## Ceedling

Logger uses Ceedling for building and testing. Standard Ceedling files
are not in GIT. These can be added by executing:

    shell> ceedling new logger

in the directory above Logger. Ceedling prompts for file
overwrites. You should answer NO in order to use the customized files.
