Darwin Streaming Server on Windows NT Server and Windows 2000 Server


Installing the server package

When the Server package is unzipped, a folder with Darwin Streaming Server
and associated files will be created. Inside this folder is an Install script,
named "Install.bat". Double-click this file to install the server and its
components on the server machine.


The Install script will create the following diretory:

c:\Program Files\Darwin Streaming Server\


Inside this directory you will find:

DarwinStreamingServer.exe	- Server executable
PlaylistBroadcaster.exe		- PlaylistBroadcaster executable
qtpasswd.exe				- Command-line utility for generating
				  				password files for access control
SpamPro.exe					- Command-line program for stress-testing 
								the Streaming Server
streamingadminserver.pl		- Admin Server that is used for administering 
								the Streaming Server
streamingserver.cfg 		- Default server configuration file
streamingrelay.cfg			- Default relay configuration file
QTSSModules/				- Folder containing QTSS API modules
Movies/						- Media folder
Logs/						- Folder containing access and error logs
AdminHtml/					- Folder containing the CGIs and the HTMl files
								required by the Admin Server
								This directory also contains the sub directories
								help/
								images/
								javascripts/
								logs/
								settings/
								status/								

Documentation/				- Documentation folder
								This directory contains the files
								AboutDarwinStreamingSvr.pdf
								WinReadMe.htm
								ReadMeNT.txt (this file)


Integration with the Service Manager

The Install script also installs Darwin Streaming Server
as a service in the Service Manager.

It is possible to start, stop, and check server status from the Service
control panel.


Server Configuration File

The Darwin Streaming Server package comes with a default
configuration file for Windows NT. The file system paths
in this configuration file are in Windows NT format.
Because Windows NT path formats are different than on UNIX
systems, a configuration file from a UNIX Darwin Streaming
Server will not work on Windows NT.


Running the Server

The server may either be started through the service control panel or from
the command-line. To start the server from the command line, open a DOS command
prompt, cd into the Darwin Streaming Server folder, and type:

DarwinStreamingServer -d

In order to get a list of the command-line options for Darwin Streaming Server, type:

DarwinStreamingServer -v


Integration with the Streaming Admin Server

The admin server is installed in c:\Program Files\Darwin Streaming Server\ along with the
streaming server. After all the files are installed, the admin server is launched from the command
prompt. The admin server needs the command window open for it to run. Closing the command window 
will cause the admin server to quit.


Running the Admin Server

If the admin server is not running, to run it, type:

perl "c:\Program Files\Darwin Streaming Server\streamingadminserver.pl" 

at the command prompt. The admin server does NOT launch the streaming server if it is not running. The server
has to be started by following the instructions for Running the Server given above.


Admin Server requirements

The admin server is written in perl. It requires Perl 5 (or above) binary for WindowsNT/2000. 
The path to the perl binary must be in the system path. If it's not, then replace "perl" above 
with the full path to the perl executable. 
For instance, if the perl binary is in c:\Program Files\Perl\perl.exe, type:

"c:\Program Files\Perl\perl.exe" "c:\Program Files\Darwin Streaming Server\streamingadminserver.pl" 

to run the admin server.

Prebuilt binary distributions of perl can be found at http://people.netscape.com/richm/nsPerl/.  


