%tagVals = (
	"scriptTag" => '=if ($ENV{"LANGUAGE"} ne "ja") {"<script type=text/javascript language=Javascript>"} else {"<script type=text/javascript language=Javascript charset=Shift_JIS>"}',
	"fontFaceCSS" => '=if ($ENV{"LANGUAGE"} ne "ja") {"Arial, Helvetica, Geneva, Swiss, sans-serif"} else {"ƒqƒ‰ƒMƒmŠpƒS Pro W3,MS PƒSƒVƒbƒN,Osaka"}',
	"qtssSvrDefaultDNSName" => "/modules/admin/server/qtssSvrDefaultDNSName",
	"qtssSvrState" => "/modules/admin/server/qtssSvrState",
	"qtssRTPSvrCurConn" => "/modules/admin/server/qtssRTPSvrCurConn",
	"qtssMP3SvrCurConn" => "/modules/admin/server/qtssMP3SvrCurConn",
	"svrCurConn" => "=<qtssobject name=qtssRTPSvrCurConn format=plaintext/> + <qtssobject name=qtssMP3SvrCurConn/>",
	"qtssSvrStartupTime" => "/modules/admin/server/qtssSvrStartupTime",
	"qtssSvrCurrentTimeMilliseconds" => "/modules/admin/server/qtssSvrCurrentTimeMilliseconds",
	"qtssRTSPSvrServerVersion" => "/modules/admin/server/qtssRTSPSvrServerVersion",
	"qtssServerAPIVersion" => "/modules/admin/server/qtssServerAPIVersion",
	"qtssElapsedTime" => "=<qtssobject name=qtssSvrCurrentTimeMilliseconds format=plaintext/> - <qtssobject name=qtssSvrStartupTime format=plaintext/>",
	"qtssSvrCPULoadPercent" => "/modules/admin/server/qtssSvrCPULoadPercent",
	"qtssRTPSvrCurBandwidth" => "/modules/admin/server/qtssRTPSvrCurBandwidth",
	"qtssMP3SvrCurBandwidth" => "/modules/admin/server/qtssMP3SvrCurBandwidth",
	"svrCurBandwidth" => "=<qtssobject name=qtssRTPSvrCurBandwidth format=plaintext/> + <qtssobject name=qtssMP3SvrCurBandwidth/>",
	"qtssRTPSvrCurConn" => "/modules/admin/server/qtssRTPSvrCurConn",
	"qtssRTPSvrTotalConn" => "/modules/admin/server/qtssRTPSvrTotalConn",
	"qtssMP3SvrTotalConn" => "/modules/admin/server/qtssMP3SvrTotalConn",
	"svrTotalConn" => "=<qtssobject name=qtssRTPSvrTotalConn format=plaintext/> + <qtssobject name=qtssMP3SvrTotalConn/>",
	"qtssRTPSvrTotalBytes" => "/modules/admin/server/qtssRTPSvrTotalBytes",
	"qtssMP3SvrTotalBytes" => "/modules/admin/server/qtssMP3SvrTotalBytes",
	"svrTotalBytes" => "=<qtssobject name=qtssRTPSvrTotalBytes format=plaintext/> + <qtssobject name=qtssMP3SvrTotalBytes/>",
	"qtssusername" => "streamingadmin",
	"startserver" => '=useDefaultIfBlank($query->{"startserver"}, $startserver)',
	"title" => "<qtssstring name=<filename/>/>",
	"pageRefreshInterval" => '=useDefaultIfBlank(getQueryOrCookie("pageRefreshInterval"), "never")',
	"displayCount" => '=useDefaultIfBlank(getQueryOrCookie("displayCount"), "all")',
	"connUserSort" => '=useDefaultIfBlank(getQueryOrCookie("connUserSort"), "ConnUserConnType_ascending")',
	"connRelaySort" => '=useDefaultIfBlank(getQueryOrCookie("relayStatSort"), "RelayStatRelayName_ascending")',
	"pageRefreshTag" => '=if ((getQueryOrCookie("pageRefreshInterval") ne "never") and (getQueryOrCookie("pageRefreshInterval") ne "")) {"<meta http-equiv=refresh content=".getQueryOrCookie("pageRefreshInterval").">"} else {"<!-- meta tag would go here -->"}',
	"qtssMovieFolder" => "/modules/admin/server/qtssSvrPreferences/movie_folder",
	"qtssMaxConn" => "/modules/admin/server/qtssSvrPreferences/maximum_connections",
	"qtssMaxThroughput" => "/modules/admin/server/qtssSvrPreferences/maximum_bandwidth",
	"qtssAuthScheme" => "/modules/admin/server/qtssSvrPreferences/authentication_scheme",
	"dialogHeader" => '=$dialogHeader',
	"dialogText" => '=$dialogText',
	"confirmMessage" => '=$confirmMessage',
	"nextFilename" => '="<input type=hidden name=filename value=\"$nextFilename\">"',
	"qtssErrorLogging" => '/modules/admin/server/qtssSvrPreferences/error_logging',
	"qtssRequestLogging" => '/modules/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/request_logging',
	"qtssErrorLogInterval" => '/modules/admin/server/qtssSvrPreferences/error_logfile_interval',
	"qtssErrorLogSize" => '/modules/admin/server/qtssSvrPreferences/error_logfile_size',
	"qtssRequestLogInterval" => '/modules/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/request_logfile_interval',
	"qtssRequestLogSize" => '/modules/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/request_logfile_size',
	"qtssMP3BroadcastPassword" => '/modules/admin/server/qtssSvrModuleObjects/QTSSMP3StreamingModule/qtssModPrefs/mp3_broadcast_password',
	"qtssErrorLog" => '=parseErrorLog()',
	"qtssIsStreamingOn80" => '=isStreamingOnPort80()',
	"qtssLogDir" => '/modules/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/request_logfile_dir',
	"qtssLogFilename" => '/modules/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/request_logfile_name',
	"qtssCurPlaylistName" => '=$curplaylist',
	"qtssCurPlaylistTitle" => '=$pltitle',
	"qtssCurPlaylistURL" => '=$plfilename',
	"qtssCurPlaylistRep" => '=$plrep',
	"qtssCurPlaylistMode" => '=$plmode',
	"pllogging" => '=$pllogging',
	"plexternal" => '=$plexternal',
	"plbroadcastip" => '=if ($plexternal eq "1") {$plbroadcastip} else {""}',
	"plbroadcastusername" => '=$plbroadcastusername',
	"plbroadcastpassword" => '=$plbroadcastpassword',
	"playlistUsernameHTML" => '=$playlistUsernameHTML',
	"currentRelay" => '=useDefaultIfBlank($query->{"currentRelay"}, $currentRelay)',
	"defaultRelayStatus" => '=getDefaultRelayStatus()',
	"relayEnabled" => '=useDefaultIfBlank($query->{"relayEnabled"}, $relayEnabled)',
	"relayType" => '=useDefaultIfBlank($query->{"relayType"}, $relayType)',
	"relayDestCount" => '=$relayDestCount',
	"relaySourceHostname" => '=useDefaultIfBlank($query->{"relaySourceHostname"}, $relaySourceHostname)',
	"relaySourceMountPoint" => '=useDefaultIfBlank($query->{"relaySourceMountPoint"}, $relaySourceMountPoint)',
	"relaySourceUsername" => '=useDefaultIfBlank($query->{"relaySourceUsername"}, $relaySourceUsername)',
	"relaySourcePassword" => '=useDefaultIfBlank($query->{"relaySourcePassword"}, $relaySourcePassword)',
	"jsstr" => '=$jsstr',
	"validateErrorField" => '=$validateErrorField',
	"validateErrorDesc" => '=$validateErrorDesc',
	"currentDest" => '=$query->{"currentDest"}',
	"qtssDestType" => '=$qtssDestType',
	"qtssDestAddr" => '=$qtssDestAddr',
	"qtssAnnouncedIP" => '=$qtssAnnouncedIP',
	"qtssUDPIP" => '=$qtssUDPIP',
	"qtssAnnouncedPorts" => '=$qtssAnnouncedPorts',
	"qtssUDPPorts" => '=$qtssUDPPorts',
	"qtssAnnouncedName" => '=$qtssAnnouncedName',
	"qtssAnnouncedPassword" => '=$qtssAnnouncedPassword',
	"qtssAnnouncedIsPath" => '=$qtssAnnouncedIsPath',
	"qtssAnnouncedPath" => '=$qtssAnnouncedPath',
	"qtssUDPIsTTL" => '=$qtssUDPIsTTL',
	"qtssUDPTTL" => '=$qtssUDPTTL',
	"doctitle" => '=$doctitle',
	"extraFieldLabel" => '=$extraFieldLabel',
	"extraFieldHTML" => '=$extraFieldHTML',
	"iteration" => '=($iteration + 1)',
	"currentdir" => '=fixPath($currentDir)',
	"filename" => '=$filename',
	"assistMP3Pass" => '=$query->{"assistMP3Pass"}',
	"assistSSL" => '=$query->{"assistSSL"}',
	"assistMovieFolder" => '=$query->{"assistMovieFolder"}',
	"adminSSL" => '=parseForSSL()',
	"SSLAvailable" => '=$ENV{"SSL_AVAIL"}',
	"playlistErrorLogText" => '=$playlistErrorLogText',
	"isMP3" => '=$isMP3',
	"chdelim" => '=getJSDelimChar()',
	"helpurl" => '=GetHelpURL()',
	"monospaceIfNotJapanese" => '=if ($ENV{"LANGUAGE"} ne "ja") {"font-family: Monaco, monospace; font-size: 10px; "} else {"font-family: font-family: Arial, Helvetica, Geneva, Swiss, sans-serif; font-size: 10px; "}',
);

@returnedKeys = ();

# edit this to include provisions for additional repeater arrays
# should return an array with the names of all of the arrays generated by this function
# any arrays generated by this function should start with 'repeater' so that there's no conflicts
sub getRepeaterArray {
	my $arrayName = $_[0];
	my $messHash = adminprotolib::GetMessageHash();
	my %messages = %$messHash;
		
	if ($arrayName eq 'connectedUsers') {
		my $connUserSort = useDefaultIfBlank(getQueryOrCookie("connUserSort"), "ConUserConnType_ascending");
		my $connUserSortOrder = 1; # default to ascending
		if ($connUserSort =~ '_descending') {
			$connUserSortOrder = 0; #descending sort order
		}
		$connUserSort =~ s/_ascending//;
		$connUserSort =~ s/_descending//;
		$connUserSortIndex = 0;
		push(@returnedKeys, ('qtssCliSesPresentationURL','fullResponse','qtssCliSesTimeConnectedinMsec','qtssCliSesPacketLossPercent','qtssCliSesRTPBytesSent','qtssCliSesCurrentBitRate','qtssCliRTSPSessRemoteAddrStr','qtssCliSesCurrentConnType'));
		$status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrClientSessions/*/*?command=get+filter1=qtssCliSesPresentationURL+filter2=QTSSReflectorModuleBroadcasterSession+filter3=qtssCliSesRTPBytesSent+filter4=qtssCliSesTimeConnectedinMsec+filter5=qtssCliSesCurrentBitRate+filter6=qtssCliRTSPSessRemoteAddrStr+filter7=qtssCliSesPacketLossPercent+filter8=QTSSRelayModuleIsRelaySession');
		my @lines = split /\r|\n/, $responseText;
		@qtssCliSesPresentationURL = ();
		@qtssCliSesTimeConnectedinMsec = ();
		@qtssCliSesPacketLossPercent = ();
		@qtssCliSesRTPBytesSent = ();
		@qtssCliSesCurrentBitRate = ();
		@qtssCliRTSPSessRemoteAddrStr = ();
		@qtssCliSesCurrentConnType = ();
		@fullResponse = ();
		push(@fullResponse, $responseText);
		my @arrayToSort = ();
		my $lineString = '';
		my $k = 0;
		my $m = 0;
		my @equivArr = ("ConnUserSession", "ConnUserConnectedTo", "ConnUserTimeConnected", "ConnUserPercPacketLoss", "ConnUserBytesSent", "ConnUserBitRate", "ConnUserIPAddress", "ConnUserConnType", "ConnUserIsRelaySession");
		# sort logic - 0=numerical, 1=case-insensitive alphabetical
		my @sortLogic = (1, 1, 0, 0, 0, 0, 1, 1, 1);
						
		my $i = 0, $j = 0, $num = 8;
		
		for ($i = 0; $i <= $#lines; $i += $num + 1)
		{
			if ($lines[$i] =~ m/Container=\"(.*)\"/gs)
			{
				# valuesArr is an array of all the values
				# [0] => QTSSReflectorModuleBroadcasterSession [1] => qtssCliSesPresentationURL [2] => qtssCliSesTimeConnectedinMsec
				# [3] => qtssCliSesPacketLossPercent [4] => qtssCliSesRTPBytesSent [5] => qtssCliSesCurrentBitRate [6] => qtssCliRTSPSessRemoteAddrStr	
				# [7] => link-to-movie-image (hardcoded) [8] => qtssRelayModuleIsRelaySession				
				my @valuesArr = ("(null)", "", 0, 0, 0, 0, "", 'icon_movie.gif', '(null)');
								
				for ($j = 0; $j < $num; $j++)
				{
					
					if($lines[$i + $j + 1] =~ m/(.*?)=\"(.*)\"/) 
					{
						if($1 eq 'QTSSReflectorModuleBroadcasterSession') {	$valuesArr[0] = $2; }
						elsif($1 eq 'qtssCliSesPresentationURL') { $valuesArr[1] = $2; }
						elsif($1 eq 'qtssCliSesTimeConnectedinMsec') { $valuesArr[2] = $2; }
						elsif($1 eq 'qtssCliSesPacketLossPercent') { $valuesArr[3] = $2; }
						elsif($1 eq 'qtssCliSesRTPBytesSent') {	$valuesArr[4] = $2; }
						elsif($1 eq 'qtssCliSesCurrentBitRate') { $valuesArr[5] = $2; }
						elsif($1 eq 'qtssCliRTSPSessRemoteAddrStr') { $valuesArr[6] = $2; }
						elsif($1 eq 'QTSSRelayModuleIsRelaySession') { $valuesArr[8] = $2; }
					}
					
				}
								
				# for sorting records...
				# convert to an array of tab-delimited strings
								
				push(@arrayToSort, join("\t", @valuesArr));

			}
			else
			{
				# badly formatted line - bail!
				last;
			}
		}
		
		# get MP3 and RTP reflected connected users
		$status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrConnectedUsers/:/*?command=get+filter1=qtssConnectionMountPoint+filter2=qtssConnectionTimeConnectedInMsec+filter3=qtssConnectionPacketLossPercent+filter4=qtssConnectionBytesSent+filter5=qtssConnectionCurrentBitRate+filter6=qtssConnectionSessRemoteAddrStr+filter7=qtssConnectionType');
		my @lines = split /\r|\n/, $responseText;
		
		$num = 7;
			
		for ($i = 0; $i <= $#lines; $i += $num + 1)
		{
			if ($lines[$i] =~ m/Container=\"(.*)\"/gs)
			{
				# valuesArr is an array of all the values
				# [0] => QTSSReflectorModuleBroadcasterSession [1] => qtssCliSesPresentationURL [2] => qtssCliSesTimeConnectedinMsec
				# [3] => qtssCliSesPacketLossPercent [4] => qtssCliSesRTPBytesSent [5] => qtssCliSesCurrentBitRate [6] => qtssCliRTSPSessRemoteAddrStr	
				# [7] => link-to-movie-image (hardcoded) [8] => qtssRelayModuleIsRelaySession				
				my @valuesArr = ("(null)", "", 0, 0, 0, 0, "", 'mp3_file.gif', '(null)');
				my $connType = '';
				
				for ($j = 0; $j < $num; $j++)
				{
					
					if($lines[$i + $j + 1] =~ m/(.*?)=\"(.*)\"/) 
					{
						if($1 eq 'qtssConnectionMountPoint') {	$valuesArr[1] = $2; }
						elsif($1 eq 'qtssConnectionTimeConnectedInMsec') { $valuesArr[2] = $2; }
						elsif($1 eq 'qtssConnectionPacketLossPercent') { $valuesArr[3] = $2; }
						elsif($1 eq 'qtssConnectionBytesSent') { $valuesArr[4] = $2; }
						elsif($1 eq 'qtssConnectionCurrentBitRate') {	$valuesArr[5] = $2; }
						elsif($1 eq 'qtssConnectionSessRemoteAddrStr') { $valuesArr[6] = $2; }
						elsif($1 eq 'qtssConnectionType') { $connType = $2; }
					}
					
				}
								
				# for sorting records...
				# convert to an array of tab-delimited strings
								
				if ($connType eq 'MP3 Client') {
					push(@arrayToSort, join("\t", @valuesArr));
				}

			}
			else
			{
				# badly formatted line - bail!
				last;
			}
		}
		
		# find out what the index of the field we're sorting is
				
		for ($i = 0; $i <= 8; $i++) {
			if ($connUserSort =~ /$equivArr[$i]/) {
				$connUserSortIndex = $i;
			}
		}
				
		# now sort the array by that index
		
		my @sortedArray = ();
		
		if ($sortLogic[$connUserSortIndex] == 0) { # numerical sort
			if ($connUserSortOrder == 1) { # ascending sort
				@sortedArray = sort {
					uc(getNthField($a, $connUserSortIndex)) <=> uc(getNthField($b, $connUserSortIndex))
				} @arrayToSort;
			}
			else { # descending sort
				@sortedArray = sort {
					uc(getNthField($b, $connUserSortIndex)) <=> uc(getNthField($a, $connUserSortIndex))
				} @arrayToSort;
			}
		}
		elsif ($sortLogic[$connUserSortIndex] == 1) { # alphabetical sort
			if ($connUserSortOrder == 1) { # ascending sort
				@sortedArray = sort {
					uc(getNthField($a, $connUserSortIndex)) cmp uc(getNthField($b, $connUserSortIndex))
				} @arrayToSort;
			}
			else { # descending sort
				@sortedArray = sort {
					uc(getNthField($b, $connUserSortIndex)) cmp uc(getNthField($a, $connUserSortIndex))
				} @arrayToSort;
			}
		}
				
		# for now copy all records - checking if broadcasterSession is not (null)
		
		# limit to x records
		
		$i = 0;
				
		foreach $item (@sortedArray) {
			@valuesArr = split(/\t/, $item);
			if (($valuesArr[0] eq '(null)') and ($valuesArr[8] eq '(null)')) {
				push(@qtssCliSesPresentationURL, $valuesArr[1]);
				push(@qtssCliSesTimeConnectedinMsec, $valuesArr[2]);
				push(@qtssCliSesPacketLossPercent, $valuesArr[3]);
				push(@qtssCliSesRTPBytesSent, $valuesArr[4]);
				push(@qtssCliSesCurrentBitRate, $valuesArr[5]);
				push(@qtssCliRTSPSessRemoteAddrStr, $valuesArr[6]);
				push(@qtssCliSesCurrentConnType, $valuesArr[7]);
				$i++;
				if ((getQueryOrCookie('displayCount') ne '') and (getQueryOrCookie('displayCount') ne 'all')) {
					if (getQueryOrCookie('displayCount') == $i) {
						last;
					}
				}
			}
		}
		
		return scalar(@qtssCliSesPresentationURL);
	}
	elsif ($arrayName eq 'relayStatuses') { # need to finish this later
		my $responseText;
		push(@returnedKeys, ('qtssRelayName','qtssRelaySource','qtssRelayDestination','qtssRelayBitrate','qtssRelayBytes'));
		my @equivArr = ('RelayStatRelayName', 'RelayStatSource', 'RelayStatDestination', 'RelayStatBitrate', 'RelayStatBytesRelayed');
		my @sortLogic = (1, 1, 1, 0, 0);
		@qtssRelayName = ();
		@qtssRelaySource = ();
		@qtssRelayDestination = ();
		@qtssRelayBitrate = ();
		@qtssRelayBytes = ();
		my $connUserSort = useDefaultIfBlank(getQueryOrCookie("connRelaySort"), "RelayStatRelayName_ascending");
		my $connUserSortOrder = 1; # default to ascending
		if ($connUserSort =~ '_descending') {
			$connUserSortOrder = 0; #descending sort order
		}
		$connUserSort =~ s/_ascending//;
		$connUserSort =~ s/_descending//;
		$connUserSortIndex = 0;
		# get the sources and put them into local arrays to match up to dests later
		my $status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrModuleObjects/QTSSRelayModule/qtssModAttributes/:/relay_session/:/*');
		my @lines = split /\r|\n/, $responseText;
		my @sourceName = ();
		my @sourceAddr = ();
		my $item = '';
		foreach $item (@lines) {
			if ($item =~ m/(.*?)=\"(.*)\"/) {
				if ($1 eq 'relay_name') { push(@sourceName, $2) }
				elsif ($1 eq 'source_ip_addr') { push(@sourceAddr, $2) }
			}
		}
		my @arrayToSort = ();
		# valuesArr: [0]qtssRelayName, [1]qtssRelaySource, [2]qtssRelayDestination, [3]qtssRelayBitrate, [4]qtssRelayBytes
		my @defaultValuesArr = ('', '', '', '', '');
		my @valuesArr = ();
		$status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrModuleObjects/QTSSRelayModule/qtssModAttributes/:/relay_session/:/relay_output/:/*');
		if ($responseText =~ /^error:\(404\)/) {
			return 0;
		}
		@lines = split /\r|\n/, $responseText;
		foreach $item (@lines) {
			if ($item =~ m/(.*?)=\"*([^\"]*)\"*/) {
				if ($1 eq 'Container') {
					if (scalar(@valuesArr) > 0) {
						push(@arrayToSort, join("\t", @valuesArr));
					}
					@valuesArr = @defaultValuesArr;
					if ($2 =~ m/\/relay_session\/([0-9]+)\//) {
						$valuesArr[0] = $sourceName[$1];
						$valuesArr[1] = $sourceAddr[$1];
					}
				}
				elsif ($1 eq 'output_dest_addr') { $valuesArr[2] = $2 }
				elsif ($1 eq 'output_cur_bitspersec') { $valuesArr[3] = $2 }
				elsif ($1 eq 'output_total_bytes_sent') { $valuesArr[4] = $2 }
			}
		}
		push(@arrayToSort, join("\t", @valuesArr));
		
		# find out what the index of the field we're sorting is
				
		for ($i = 0; $i <= scalar(@equivArr); $i++) {
			if ($connUserSort =~ /$equivArr[$i]/) {
				$connUserSortIndex = $i;
			}
		}
				
		# now sort the array by that index
				
		my @sortedArray = ();
		
		if ($sortLogic[$connUserSortIndex] == 0) { # numerical sort
			if ($connUserSortOrder == 1) { # ascending sort
				@sortedArray = sort {
					uc(getNthField($a, $connUserSortIndex)) <=> uc(getNthField($b, $connUserSortIndex))
				} @arrayToSort;
			}
			else { # descending sort
				@sortedArray = sort {
					uc(getNthField($b, $connUserSortIndex)) <=> uc(getNthField($a, $connUserSortIndex))
				} @arrayToSort;
			}
		}
		elsif ($sortLogic[$connUserSortIndex] == 1) { # alphabetical sort
			if ($connUserSortOrder == 1) { # ascending sort
				@sortedArray = sort {
					uc(getNthField($a, $connUserSortIndex)) cmp uc(getNthField($b, $connUserSortIndex))
				} @arrayToSort;
			}
			else { # descending sort
				@sortedArray = sort {
					uc(getNthField($b, $connUserSortIndex)) cmp uc(getNthField($a, $connUserSortIndex))
				} @arrayToSort;
			}
		}
				
		# for now copy all records - checking if broadcasterSession is not (null)
		
		# limit to x records
		
		$i = 0;
				
		foreach $item (@sortedArray) {
			@valuesArr = split(/\t/, $item);
			push(@qtssRelayName, $valuesArr[0]);
			push(@qtssRelaySource, $valuesArr[1]);
			push(@qtssRelayDestination, $valuesArr[2]);
			push(@qtssRelayBitrate, $valuesArr[3]);
			push(@qtssRelayBytes, $valuesArr[4]);
			$i++;
			if ((getQueryOrCookie('displayCount') ne '') and (getQueryOrCookie('displayCount') ne 'all')) {
				if (getQueryOrCookie('displayCount') == $i) {
					last;
				}
			}
		}
		
		return scalar(@qtssRelayName);
	}
	elsif ($arrayName eq 'pathlist') {
		push(@returnedKeys, ('dirname','dirpath'));
		@dirname = ();
		@dirpath = ();
		
		$chdelim = &playlistlib::GetFileDelimChar();
		if ($chdelim eq '\\') {
			$chdelim = '\\\\';
		}
		my $currentdirfull = useDefaultIfBlank($currentDir, $movieDir);
		while ($currentdirfull =~ m/(.*)$chdelim(.*)/s) {
			push(@dirname, $2);
			push(@dirpath, $currentdirfull);
			if ($currentdirfull eq $movieDir) {
				last;
			}
			else {
				$currentdirfull = $1;
			}
		}
		return scalar(@dirname);
	}
	elsif ($arrayName eq 'dirlisting') {
		my $i = 0;
		my $fixedchdelim = getJSDelimChar();
		my $testfn = '';
		my $hiddenFilenames = '|Move&Rename|Cleanup At Startup|Desktop DB|Desktop DF|Desktop|Network Trash Folder|Shutdown Check|Temporary Items|TheFindByContentFolder|TheVolumeSettingsFolder|qtusers|qtgroups|qtaccess|';
		push(@returnedKeys, ('qtssIndividualFileName', 'qtssFileIcon'));
		@qtssIndividualFileName = ();
		@qtssFileIcon = ();
		my $currentDirFixed = $currentDir;
		$currentDirFixed =~ s/\\/\\\\/g;
		my $chdelim = &playlistlib::GetFileDelimChar();
		if(opendir(DIR, useDefaultIfBlank(getQueryOrCookie('submitcurrentdir'), $currentDir))) {
			while (defined($file = readdir(DIR))) {
				$doContinue = 1;
				if ($hiddenFilenames =~ m/\|$file\|/o) {
					next;
				}
				if ($isMP3 and (not ($file =~ /.[Mm][Pp]3$/)) and (not (-d "$currentDir$chdelim$file"))) {
					next;
				}
				if (not ($isMP3) and ($file =~ /.[Mm][Pp]3$/) and (not (-d "$currentDir$chdelim$file"))) {
					next;
				}
				if(not (($file =~ /^\./) or ($file =~ /.[Ss][Dd][Pp]/) or ($file =~ /[\r\n]/))) {
					$file =~ s/\\/\\\\/g;
					$file =~ s/'/\\'/g;
					push(@qtssIndividualFileName, "$currentDirFixed$fixedchdelim$file");
					if(-d "$currentDir$chdelim$file") {
						push(@qtssFileIcon, 'images/icon_folder.gif');
					}
					else {
						$testfn = lc($file);
						if ($testfn =~ /.mp3/) {
							push(@qtssFileIcon, 'images/mp3_file.gif');
						}
						elsif (($testfn =~ /.mov/) or ($testfn =~ /.mpg/) or ($testfn =~ /.mp4/) or ($testfn =~ /.mpeg/) or ($testfn =~ /.avi/)) {
							push(@qtssFileIcon, 'images/icon_movie.gif');
						}
						else {
							push(@qtssFileIcon, 'images/icon_generic.gif');
						}
					}
				}
			}
		}
		# join into a list of tab-delimited strings so we can sort it
		my @sortedArray = ();
		for ($i = 0; $i <= $#qtssIndividualFileName; $i++) {
			push(@sortedArray, $qtssIndividualFileName[$i]."\t".$qtssFileIcon[$i])
		}
		@sortedArray = sort {uc($a) cmp uc($b)} @sortedArray;
		@qtssIndividualFileName = ();
		@qtssFileIcon = ();
		my @splitItem = ();
		#split it back out here
		foreach $item (@sortedArray) {
			@splitItem = split(/\t/, $item);
			push(@qtssIndividualFileName, $splitItem[0]);
			push(@qtssFileIcon, $splitItem[1]);
		}
		return scalar(@qtssIndividualFileName);
	}
	elsif ($arrayName eq 'accesslog') {
		push(@returnedKeys, ('qtssAccessURI','qtssAccessCount'));
		@qtssAccessURI = ();
		@qtssAccessCount = ();
		my @countHash = ();
		my $returnText = '';
		my $thisIP = '';
		my $line = '';
		my $i = 0;
		my $item = '';
		my $foundIt = 'false';
		my $status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/request_logfile_dir');
		$_ = $responseText;
		if (!(/request_logfile_dir="([^"]+)"/)) {
			die 'Error getting directory.';
		}
		my $dirname = $1;
		$status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrModuleObjects/QTSSAccessLogModule/qtssModPrefs/request_logfile_name');
		$_ = $responseText;
		if (!(/request_logfile_name="([^"]+)"/)) {
			die 'Error getting filename.';
		}
		$dirname .= '/' . $1 . '.log';
		open(LOGFILE, $dirname) or return "Can't open log file '$dirname'!";
		while ($line = <LOGFILE>) {
			if (!($line =~ /^#/)) {
				if ($line =~ /^[^\s]*\s*[^\s]*\s*[^\s]*\s*[^\s]*\s*([^\s]*)/) {
					$foundIt = 'false';
					$i = 0;
					for ($i = 0; $i <= $#qtssAccessURI; $i++) {
						if ($qtssAccessURI[$i] eq $1) {
							$qtssAccessCount[$i]++;
							$foundIt = 'true';
							last;
						}
					}
					if ($foundIt ne 'true') {
						push(@qtssAccessURI, $1);
						push(@qtssAccessCount, 1);
					}
				}
			}
		}
		close(LOGFILE);
		return scalar(@qtssAccessURI);
	}
	elsif ($arrayName eq 'playlists') {
		push(@returnedKeys, ('qtssPlaylistNames','qtssPlaylistTitles','qtssPlaylistPaths','qtssPlaylistStatuses','qtssPlaylistImages'));
		@qtssPlaylistNames = ();
		@qtssPlaylistTitles = ();
		@qtssPlaylistStatuses = ();
		@qtssPlaylistImages = ();
		@qtssPlaylistPaths = ();
		my $dir = &playlistlib::GetPLRootDir();
		my $chdelim = &playlistlib::GetFileDelimChar();
		
		if (!(-e "$dir")) {
			# directory doesn't exist; 
			# this is probably an error.
			mkdir "$dir", 0770;
		}
		if (opendir(DIR, $dir)) {
			while( defined ($name = readdir DIR) ) {
				# print all the subdirectories in $plroot.
				if (!(-f "$dir$name") && ($name !~ /^[.]+/)) { 
					push(@qtssPlaylistStatuses, &playlistlib::GetPlayListState($name));
					my $playlistdataref = &playlistlib::ParsePlayListEntry($name);
					my @playlistdata = @$playlistdataref;
					push(@qtssPlaylistNames, $name);
					push(@qtssPlaylistTitles, $playlistdata[9]);
					push(@qtssPlaylistPaths, "$dir$name$chdelim");
					if ($playlistdata[5] eq '') { # movie playlist
						push(@qtssPlaylistImages, "icon_movie.gif");
					}
					else { # mp3 playlist
						push(@qtssPlaylistImages, "mp3_file.gif");
					}
				}
			}
			closedir(DIR);
		}
		return scalar(@qtssPlaylistNames);
	}
	elsif ($arrayName eq 'playlistitems') {
		$status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrPreferences/movie_folder');
		$_ = $responseText;
		if (!(/movie_folder="([^"]+)"/)) {
			die 'Error getting movie folder.';
		}
		my $movie_folder = $1 . &playlistlib::GetFileDelimChar();
		push(@returnedKeys, ('qtssPlaylistItemName','qtssPlaylistItemWeight'));
		@qtssPlaylistItemName = ();
		@qtssPlaylistItemWeight = ();
		foreach $item (@plfiles) {
			if ($item =~ /(.+)[:]([0-9]+)$/) {
				$itemName = $1;
				$itemWeight = $2;
				$itemName =~ s/\\/\\\\/g;
				$itemName =~ s/\'/\\\'/g;
				push(@qtssPlaylistItemName, $itemName);
				push(@qtssPlaylistItemWeight, $itemWeight);
			}
		}
		return scalar(@qtssPlaylistItemName);
	}
	elsif ($arrayName eq 'relaynames') {
		my $status = adminprotolib::EchoData($relayConfigDir, $messHash, $authheader, $qtssip, $qtssport, "/modules/admin/server/qtssSvrModuleObjects/QTSSRelayModule/qtssModPrefs/relay_prefs_file", "/modules/admin/server/qtssSvrModuleObjects/QTSSRelayModule/qtssModPrefs/relay_prefs_file");
		my $relayarrayref = getArraysFromFile($relayConfigDir);
		my $sourcehashref;
		my %sourcehash;
		my @relays = @$relayarrayref;
		my $i = 0;
		push(@returnedKeys, ('qtssRelayName','qtssRelayStatus'));
		@qtssRelayName = ();
		@qtssRelayStatus = ();
		$defaultRelayName = $messages{'RelayDefaultRelayName'};
		foreach $relayRef (@relays) {
			@relay = @$relayRef;
			$sourcehashref = $relay[2];
			%sourcehash = %$sourcehashref;
			if (($sourcehash{'type'} ne 'udp_source') and ($sourcehash{'source_addr'} ne '')) {
				push(@qtssRelayName, $relay[0]);
				if ($relay[1] == 1) {
					push(@qtssRelayStatus, $messages{'RelayStatusEnabled'});
				}
				else {
					push(@qtssRelayStatus, $messages{'RelayStatusDisabled'});
				}
			}
		}
		return scalar(@qtssRelayName);
	}
	elsif ($arrayName eq 'relaydests') {
		# this repeater expects you to call getValsForRelay() first
		push(@returnedKeys, ('relayDestHostname','relayDestMountPoint','relayDestType','relayDestUsername','relayDestPassword','relayDestPort','relayDestTTL'));
		return $relayDestCount;
	}
	elsif ($arrayName eq 'extragensettings') {
		@qtssExtraGenSettings = ();
		@qtssExtraGenLabels = ();
		@qtssExtraGenDescs = ();
		push(@returnedKeys, ('qtssExtraGenSettings','qtssExtraGenLabels','qtssExtraGenDescs'));
		if($^O eq "darwin") {
			push(@qtssExtraGenSettings, parseForAutostart());
			push(@qtssExtraGenLabels, $messages{'GenSetOSXAutoStart'});
			push(@qtssExtraGenDescs, $messages{'Enabled'});
		}
		return scalar(@qtssExtraGenSettings);
	}
	elsif ($arrayName eq 'queryparams') {
		@passthrough = ();
		my %queryHash = %$query;
		my $key = '';
		push(@returnedKeys, 'passthrough');
		foreach $key (keys %queryHash) {
			if (($key ne 'filename') and ($key ne 'action')) {
				push(@passthrough, "<input type=hidden name=\"$key\" value=\"".$queryHash{$key}."\">");
			}
		}
		return scalar(@passthrough);
	}
}

# use this to sub a default value (second parameter) if the first parameter is empty
sub useDefaultIfBlank {
	my $realValue = $_[0];
	my $defaultValue = $_[1];
	
	if ($realValue eq '') {
		return $defaultValue;
	}
	else {
		return $realValue;
	}
}

sub getCookie {
	$_ = $ENV{"COOKIES"};
	my $matchVal = $_[0];
	if (/$matchVal=[^;]*/) {
		$_ = $&;
		s/$matchVal=//;
		return $_;
	}
	else {
		return '';
	}
}

sub getQueryOrCookie {
	my $theName = $_[0];
	
	if ($query->{$theName} eq '') {
		return getCookie($theName);
	}
	else {
		return $query->{$theName};
	}
}

sub parseErrorLog {
	my $chdelim = &playlistlib::GetFileDelimChar();
	my $messHash = adminprotolib::GetMessageHash();	
	my $returnText = '';
	my $status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrPreferences/error_logfile_dir');
	$_ = $responseText;
	if (!(/error_logfile_dir="([^"]+)"/)) {
		return 'Error getting directory.';
	}
	my $dirname = $1;
	$status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrPreferences/error_logfile_name');
	$_ = $responseText;
	if (!(/error_logfile_name="([^"]+)"/)) {
		return 'Error getting filename.';
	}
	$dirname .= $chdelim . $1 . '.log';
	open(LOGFILE, $dirname) or return "";
	while ($line = <LOGFILE>) {
		$_ = $line;
		if (/^\#/) { # comment or something
			if (/^\#Log/) {  # top header
				$line = '<span class=logheader>'.$line.'</span><dl>';
			}
			else { # any other comment
				$line =~ s/STARTUP/\<span class=green\>STARTUP\<\/span\>/;
				$line =~ s/SHUTDOWN/\<span class=red\>SHUTDOWN\<\/span\>/;
				$line = '</dl><b>'.$line.'</b><dl>';
			}
		}
		else {
			$line = '<dd>'.$line.'</dd>';
		}
		$returnText .= $line;
	}
	$returnText .= '</dl>';
	close(LOGFILE);
	return $returnText;
}

sub parseForSSL {
	my $configFilePath = $ENV{"QTSSADMINSERVER_CONFIG"};
	my $line = '';
	my $fulltext = '';
	
	open(CONFIGFILE, "<$configFilePath") or return 0;
	while($line = <CONFIGFILE>) {
		if ($line =~ /ssl=1/) {
			close(CONFIGFILE);
			return '1';
		}
	}
	close(CONFIGFILE);
	return '0';
}

sub parseForAutostart {
	my $configFilePath = $ENV{"QTSSADMINSERVER_CONFIG"};
	my $line = '';
	my $fulltext = '';
	
	open(CONFIGFILE, "<$configFilePath") or return 0;
	while($line = <CONFIGFILE>) {
		if ($line =~ /qtssAutoStart=([0-1])/) {
			close(CONFIGFILE);
			return $1;
		}
	}
	close(CONFIGFILE);
	return '1';
}

sub isStreamingOnPort80 {
	my $messHash = adminprotolib::GetMessageHash();	
	my $status = adminprotolib::GetData($responseText, $messHash, $authheader, $qtssip, $qtssport, '/modules/admin/server/qtssSvrPreferences/rtsp_port/*');
	if ($responseText =~ /[0-9]="80"/) {
		return "true";
	}
	else {
		return "false";
	}
}

sub getDefaultRelayStatus {
	my $messHash = adminprotolib::GetMessageHash();
	my %messages = %$messHash;
	my $status = adminprotolib::EchoData($relayConfigDir, $messHash, $authheader, $qtssip, $qtssport, "/modules/admin/server/qtssSvrModuleObjects/QTSSRelayModule/qtssModPrefs/relay_prefs_file", "/modules/admin/server/qtssSvrModuleObjects/QTSSRelayModule/qtssModPrefs/relay_prefs_file");
	my $relayarrayref = getArraysFromFile($relayConfigDir);
	my $sourcehashref;
	my %sourcehash;
	my @relays = @$relayarrayref;
	foreach $relayRef (@relays) {
		@relay = @$relayRef;
		$sourcehashref = $relay[2];
		%sourcehash = %$sourcehashref;
		if ($sourcehash{'source_addr'} eq '') {
			if ($relay[1] == 1) {
				return $messages{'RelayStatusEnabled'};
			}
			else {
				return $messages{'RelayStatusDisabled'};
			}
		}
	}
}

sub getNthField {
	$theString = $_[0];
	$theFieldNum = $_[1];
	my @tempSplitString = split(/\t/, $theString);
	return $tempSplitString[$theFieldNum];
}

sub getJSDelimChar {
	my $theDelim = &playlistlib::GetFileDelimChar();
	if ($theDelim eq '\\') {
		return '\\\\';
	}
	else {
		return $theDelim;
	}
}

sub fixPath {
	my $newPath = $_[0];
	if (&playlistlib::GetFileDelimChar() eq '\\') {
		$newPath =~ s/\\/\\\\/g;
	}
	return $newPath;
}

# GetHelpURL()
# Returns the help URL for the current language.
sub GetHelpURL
{
	my $lang = $ENV{"LANGUAGE"};

	if ($^O eq 'darwin') {
		if ($lang eq 'en') {
			return 'http://helpqt.apple.com/qtssWebAdminHelpR3/qtssWebAdmin.help/English.lproj/QTSSHelp.htm';
		}
		elsif ($lang eq 'de') {
			return 'http://helpqt.apple.com/qtssWebAdminHelpR3/qtssWebAdmin.help/German.lproj/QTSSHelp.htm';
		}
		elsif ($lang eq 'ja') {
			return 'http://helpqt.apple.com/qtssWebAdminHelpR3/qtssWebAdmin.help/Japanese.lproj/QTSSHelp.htm';
		}
		elsif ($lang eq 'fr') {
			return 'http://helpqt.apple.com/qtssWebAdminHelpR3/qtssWebAdmin.help/French.lproj/QTSSHelp.htm';
		}
	}
	else {
		return 'http://helpqt.apple.com/dssWebAdminHelpR3/dssWebAdmin.help/DSSHelp.htm';
	}
}

# MacQTGroupsContainsAdminGroup()
# returns 1 if qtgroups exists and contains admin group,
# or 0 if it doesn't
sub MacQTGroupsContainsAdminGroup
{
	if (-e '/Library/QuickTimeStreaming/Config/qtgroups') {
		my $line = '';
		
		open(GROUPSFILE, '/Library/QuickTimeStreaming/Config/qtgroups') or return 0;
		while ($line = <GROUPSFILE>) {
			if ($line =~ /^admin:/o) {
				close(GROUPSFILE);
				return 1;
			}
		}
		close(GROUPSFILE);
		return 0;
	}
	else {
		return 0;
	}
}

1; # return true