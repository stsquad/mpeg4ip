mkdir "c:\Program Files\Darwin Streaming Server\"

copy DarwinStreamingServer.exe "c:\Program Files\Darwin Streaming Server\DarwinStreamingServer.exe"
copy streamingserver.cfg  "c:\Program Files\Darwin Streaming Server\streamingserver.cfg"
copy streamingrelay.cfg  "c:\Program Files\Darwin Streaming Server\streamingrelay.cfg"
copy qtpasswd.exe  "c:\Program Files\Darwin Streaming Server\qtpasswd.exe"
copy PlaylistBroadcaster.exe  "c:\Program Files\Darwin Streaming Server\PlaylistBroadcaster.exe"
copy SpamPro.exe  "c:\Program Files\Darwin Streaming Server\SpamPro.exe"
copy spampro.conf  "c:\Program Files\Darwin Streaming Server\spampro.conf"
copy streamingserver.xml "c:\Program Files\Darwin Streaming Server\streamingserver.xml-sample"

mkdir  "c:\Program Files\Darwin Streaming Server\QTSSModules\"

REM copy QTSSAccessModule.dll "c:\Program Files\Darwin Streaming Server\QTSSModules\QTSSAccessModule.dll"
REM copy QTSSRawFileModule.dll "c:\Program Files\Darwin Streaming Server\QTSSModules\QTSSRawFileModule.dll"
REM copy QTSSSpamDefenseModule.dll "c:\Program Files\Darwin Streaming Server\QTSSModules\QTSSSpamDefenseModule.dll"

mkdir  "c:\Program Files\Darwin Streaming Server\Movies\"
copy sample.mov "c:\Program Files\Darwin Streaming Server\Movies\sample.mov"

mkdir  "c:\Program Files\Darwin Streaming Server\Logs\"

mkdir "c:\Program Files\Darwin Streaming Server\Documentation\"
copy ReadMeNT.txt "c:\Program Files\Darwin Streaming Server\Documentation"
copy WinReadMe.htm "c:\Program Files\Darwin Streaming Server\Documentation"
copy AboutDarwinStreamingSvr.pdf "c:\Program Files\Darwin Streaming Server\Documentation"

copy streamingadminserver.pl "c:\Program Files\Darwin Streaming Server\streamingadminserver.pl"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\"
copy AdminHtml\*.pl "c:\Program Files\Darwin Streaming Server\AdminHtml"
copy AdminHtml\*.cgi "c:\Program Files\Darwin Streaming Server\AdminHtml"
copy AdminHtml\mainpage.htm "c:\Program Files\Darwin Streaming Server\AdminHtml"
copy AdminHtml\nav.htm "c:\Program Files\Darwin Streaming Server\AdminHtml"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\images\"
copy AdminHtml\images\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\images"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\javascripts\"
copy AdminHtml\javascripts\*.js "c:\Program Files\Darwin Streaming Server\AdminHtml\javascripts"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\logs\"
copy AdminHtml\logs\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\logs"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\settings\"
copy AdminHtml\settings\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\settings"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\status\"
copy AdminHtml\status\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\status"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\help\"
copy HelpFiles\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\help"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\help\gfx\"
copy HelpFiles\gfx\*.gif "c:\Program Files\Darwin Streaming Server\AdminHtml\help\gfx"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\help\pgs\"
copy HelpFiles\pgs\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\help\pgs"

mkdir "c:\Program Files\Darwin Streaming Server\AdminHtml\help\shrd\"
copy HelpFiles\shrd\*.htm "c:\Program Files\Darwin Streaming Server\AdminHtml\help\shrd"


"c:\Program Files\Darwin Streaming Server\DarwinStreamingServer" -i

perl "c:\Program Files\Darwin Streaming Server\streamingadminserver.pl"

pause