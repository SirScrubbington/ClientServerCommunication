# ClientServerCommunication

A Client-Server communication application written in the C programming language for Unix Operating Systems. By default, the client and server both attempt to connect to port 80. 

# Commands:
List [-l] [-f] [pathname] [localfile]
Prints a list of all the files in the current directory or at ‘pathname’ to stdout or ‘localfile’.

Get filepath [-f] [localfile]
Print content of ‘filepath’ to stdout or ‘localfile’

Put localfile [-f] [newname]
Create remote copy of local file with same or other name

Sys
Display name / version of OS and CPU Type

Delay Integer
Returns ‘integer’ after a delay of ‘integer’ seconds.

Quit
Closes the client program.

The “-l” argument for list, if provided also returns the file size, owner, creation date
and access permissions of the file.

The “-f” argument if provided forces any files which are created by the program to ovewrite
existing files. If this argument is not provided and an existing file is needed to be
overwritten, the write will fail.

If no filename is provided for list, the directory listing is printed to the screen forty
lines at a time.
