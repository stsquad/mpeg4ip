echo off
mkdir "c:\Program Files\Darwin Streaming Server\"

copy DarwinStreamingServer.exe "c:\Program Files\Darwin Streaming Server\DarwinStreamingServer.exe"
copy streamingserver.xml "c:\Program Files\Darwin Streaming Server\streamingserver.xml-sample"
copy streamingserver.xml "c:\Program Files\Darwin Streaming Server\streamingserver.xml"
copy qtusers "c:\Program Files\Darwin Streaming Server\qtusers"
copy streamingrelay.cfg  "c:\Program Files\Darwin Streaming Server\streamingrelay.cfg"
copy PlaylistBroadcaster.exe  "c:\Program Files\Darwin Streaming Server\PlaylistBroadcaster.exe"
copy streamingadminserver.pl "c:\Program Files\Darwin Streaming Server\streamingadminserver.pl"
copy streamingadminserver.pem "c:\Program Files\Darwin Streaming Server\streamingadminserver.pem"
copy qtpasswd.exe  "c:\Program Files\Darwin Streaming Server\qtpasswd.exe"
del /Q "c:\Program Files\Darwin Streaming Server\SpamPro.exe"
copy StreamingLoadTool.exe  "c:\Program Files\Darwin Streaming Server\StreamingLoadTool.exe"
copy streamingloadtool.cfg  "c:\Program Files\Darwin Streaming Server\streamingloadtool.cfg"

mkdir  "c:\Program Files\Darwin Streaming Server\QTSSModules\"
REM copy QTSSRawFileModule.dll "c:\Program Files\Darwin Streaming Server\QTSSModules\QTSSRawFileModule.dll"
REM copy QTSSSpamDefenseModule.dll "c:\Program Files\Darwin Streaming Server\QTSSModules\QTSSSpamDefenseModule.dll"

mkdir  "c:\Program Files\Darwin Streaming Server\Playlists\"

mkdir  "c:\Program Files\Darwin Streaming Server\Movies\"
copy sample.mov "c:\Program Files\Darwin Streaming Server\Movies\sample.mov"

mkdir  "c:\Program Files\Darwin Streaming Server\Logs\"

rmdir /S /Q "c:\Program Files\Darwin Streaming Server\Documentation"
mkdir "c:\Program Files\Darwin Streaming Server\Documentation\"
copy ReadMeNT.txt "c:\Program Files\Darwin Streaming Server\Documentation"
copy WinReadMe.htm "c:\Program Files\Darwin Streaming Server\Documentation"
copy AboutDarwinStreamingSvr.pdf "c:\Program Files\Darwin Streaming Server\Documentation"

rmdir /S /Q "c:Program Files\Darwin Streaming Server\AdminHtml"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\"
copy AdminHtml\*.pl "c:\Program Files\Darwin Streaming Server\AdminHtml"
copy AdminHtml\*.cgi "c:\Program Files\Darwin Streaming Server\AdminHtml"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\images\"
copy AdminHtml\images\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\images"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\javascripts\"
copy AdminHtml\javascripts\*.js "c:\Program Files\Darwin Streaming Server\AdminHtml\javascripts"


mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\"
copy AdminHtml\html_en\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en"
copy AdminHtml\html_en\messages "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\logs\"
copy AdminHtml\html_en\logs\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\logs"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\settings\"
copy AdminHtml\html_en\settings\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\settings"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\status\"
copy AdminHtml\html_en\status\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\status"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\images\"
copy AdminHtml\html_en\images\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\html_en\images"


mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\"
copy AdminHtml\html_de\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de"
copy AdminHtml\html_de\messages "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\logs\"
copy AdminHtml\html_de\logs\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\logs"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\settings\"
copy AdminHtml\html_de\settings\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\settings"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\status\"
copy AdminHtml\html_de\status\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\status"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\images\"
copy AdminHtml\html_de\images\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\html_de\images"


mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\"
copy AdminHtml\html_fr\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr"
copy AdminHtml\html_fr\messages "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\logs\"
copy AdminHtml\html_fr\logs\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\logs"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\settings\"
copy AdminHtml\html_fr\settings\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\settings"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\status\"
copy AdminHtml\html_fr\status\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\status"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\images\"
copy AdminHtml\html_fr\images\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\html_fr\images"


mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\"
copy AdminHtml\html_ja\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja"
copy AdminHtml\html_ja\messages "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\logs\"
copy AdminHtml\html_ja\logs\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\logs"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\settings\"
copy AdminHtml\html_ja\settings\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\settings"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\status\"
copy AdminHtml\html_ja\status\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\status"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\images\"
copy AdminHtml\html_ja\images\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\html_ja\images"


mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\help\"
copy HelpFiles\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\help"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\help\gfx\"
copy HelpFiles\gfx\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\help\gfx"
copy HelpFiles\gfx\*.jpg "c:\Program Files\Darwin Streaming Server\AdminHtml\help\gfx"
mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\help\pgs\"
copy HelpFiles\pgs\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\help\pgs"


"c:\Program Files\Darwin Streaming Server\DarwinStreamingServer" -i

echo Launching Streaming Admin Server
perl "c:\Program Files\Darwin Streaming Server\streamingadminserver.pl"
echo -----------------------------------------------------------------
echo Perl is not properly installed on your system.
echo Please make sure that the path to the Perl interpreter
echo is in your system path and run 
echo c:\Program Files\Darwin Streaming Server\streamingadminserver.pl
echo from the command prompt.

pause
