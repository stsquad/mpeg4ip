mkdir DarwinStreamingServer
copy DarwinStreamingServer.exe DarwinStreamingServer\DarwinStreamingServer.exe
copy PlaylistBroadcaster.exe DarwinStreamingServer\PlaylistBroadcaster.exe
copy SpamPro.exe DarwinStreamingServer\SpamPro.exe
copy streamingserver.xml DarwinStreamingServer\streamingserver.xml
copy ..\SpamPro.tproj\spampro.conf DarwinStreamingServer\spampro.conf
copy qtpasswd.exe DarwinStreamingServer\qtpasswd.exe
copy Install.bat DarwinStreamingServer\Install.bat
copy streamingrelay.cfg DarwinStreamingServer\streamingrelay.cfg
copy streamingserver.cfg DarwinStreamingServer\streamingserver.cfg
copy ..\sample.mov DarwinStreamingServer\sample.mov
copy dynmodules\QTSSAccessModule.dll DarwinStreamingServer\QTSSAccessModule.dll
copy dynmodules_disabled\QTSSSpamDefenseModule.dll DarwinStreamingServer\QTSSSpamDefenseModule.dll
copy dynmodules_disabled\QTSSRawFileModule.dll DarwinStreamingServer\QTSSRawFileModule.dll
copy ReadMeNT.txt DarwinStreamingServer\ReadMeNT.txt
copy ..\Documentation\WinReadMe.htm DarwinStreamingServer\WinReadMe.htm
copy ..\Documentation\AboutDarwinStreamingSvr.pdf DarwinStreamingServer\AboutDarwinStreamingSvr.pdf


mkdir DarwinStreamingServer\HelpFiles
copy /Y ..\Documentation\HelpFiles\*.htm DarwinStreamingServer\HelpFiles

mkdir DarwinStreamingServer\HelpFiles\gfx
copy /Y ..\Documentation\HelpFiles\gfx\*.gif DarwinStreamingServer\HelpFiles\gfx

mkdir DarwinStreamingServer\HelpFiles\pgs
copy /Y ..\Documentation\HelpFiles\pgs\*.htm DarwinStreamingServer\HelpFiles\pgs

mkdir DarwinStreamingServer\HelpFiles\shrd
copy /Y ..\Documentation\HelpFiles\shrd\*.htm DarwinStreamingServer\HelpFiles\shrd



copy ..\WebAdmin\src\streamingadminserver.pl DarwinStreamingServer\streamingadminserver.pl

mkdir DarwinStreamingServer\AdminHtml
copy /Y ..\WebAdmin\WebAdminHtml\*.pl DarwinStreamingServer\AdminHtml
copy /Y ..\WebAdmin\WebAdminHtml\*.cgi DarwinStreamingServer\AdminHtml
copy /Y ..\WebAdmin\WebAdminHtml\mainpage.htm DarwinStreamingServer\AdminHtml
copy /Y ..\WebAdmin\WebAdminHtml\nav.htm DarwinStreamingServer\AdminHtml

mkdir DarwinStreamingServer\AdminHtml\images
copy /Y ..\WebAdmin\WebAdminHtml\images\*.gif DarwinStreamingServer\AdminHtml\images

mkdir DarwinStreamingServer\AdminHtml\javascripts
copy /Y ..\WebAdmin\WebAdminHtml\javascripts\*.js DarwinStreamingServer\AdminHtml\javascripts

mkdir DarwinStreamingServer\AdminHtml\logs
copy /Y ..\WebAdmin\WebAdminHtml\logs\*.htm DarwinStreamingServer\AdminHtml\logs

mkdir DarwinStreamingServer\AdminHtml\settings
copy /Y ..\WebAdmin\WebAdminHtml\settings\*.htm DarwinStreamingServer\AdminHtml\settings

mkdir DarwinStreamingServer\AdminHtml\status
copy /Y ..\WebAdmin\WebAdminHtml\status\*.htm DarwinStreamingServer\AdminHtml\status