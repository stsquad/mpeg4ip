// status_connected.js
// all javascript functions for the files
// status_connected.htm, status_connected_xbit.htm, status_connected_xbytes.htm, status_connected_xip.htm,
// status_connected_xloss.htm, status_connected_xmovie.htm, status_connected_xtime.htm
  	function updateForm() {
  		top.nav.numEntriesConnectedUsers = document.forms[0].numEntries.selectedIndex;
  		top.nav.filenameConnectedUsers = setUrl();
		document.forms[0].submit();
	}
	function doNothing() {
		// just set the value in the top frame to preserve the value
		top.nav.sortOrderConnectedUsers = document.forms[0].sortOrder.selectedIndex;
		top.nav.filenameConnectedUsers = setUrl(); 
	}
	function redirectPage(file) {
		var entriesInt, refreshInt, sortInt;
		entriesInt = document.forms[0].numEntries.selectedIndex;
		refreshInt = document.forms[0].refreshInterval.selectedIndex;
		sortInt = document.forms[0].sortOrder.selectedIndex;
		//var num = document.forms[0].numEntries.options[entriesInt].text;
		//var ref = document.forms[0].refreshInterval.options[refreshInt].text;
		//var order = document.forms[0].sortOrder.options[sortInt].text;
		//var url = "parsewithinput.cgi?filename="+file+"&numEntries="+num+"&refreshInterval="+ref+"&sortOrder="+order;
		var url = "parsewithinput.cgi?filename="+file+"&numEntries="+entriesInt+"&refreshInterval="+refreshInt+"&sortOrder="+sortInt;
		document.location = url;
	}	
	function setUrl() {
		var entriesInt, refreshInt, sortInt, file;
		file = document.forms[0].filename.value;
		entriesInt = document.forms[0].numEntries.selectedIndex;
		refreshInt = document.forms[0].refreshInterval.selectedIndex;
		sortInt = document.forms[0].sortOrder.selectedIndex;
		//var num = document.forms[0].numEntries.options[entriesInt].text;
		//var ref = document.forms[0].refreshInterval.options[refreshInt].text;
		//var order = document.forms[0].sortOrder.options[sortInt].text;
		//var url = "parsewithinput.cgi?filename="+file+"&numEntries="+num+"&refreshInterval="+ref+"&sortOrder="+order;
		var url = "parsewithinput.cgi?filename="+file+"&numEntries="+entriesInt+"&refreshInterval="+refreshInt+"&sortOrder="+sortInt;
		return url;
	}
	function startTimer() {
		// before start timer set the value in the top frame
		top.nav.refreshIntervalConnectedUsers = document.forms[0].refreshInterval.selectedIndex;
		top.nav.filenameConnectedUsers = setUrl();
    	var interval;
    	clearTimeout(document.forms[0].timeoutId.value);
    	switch(document.forms[0].refreshInterval.selectedIndex) {
    		case 0: interval = 10000; break;
    		case 1: interval = 60000; break;
    		case 2: interval = 300000; break;
    		case 3: interval = 1800000; break;
    		case 4: interval = 3600000; break;
    		case 5: interval = -1; break;
    	}
    	if(interval != -1) {
    		document.forms[0].timeoutId.value = setTimeout('refresh()', interval);
		}
	}
	function init() {
		toploded = top.nav.loded;
		if(typeof(toploded) == "number") {
			top.nav.selekt('1');
			top.nav.subSelekt('1B');
		} 
		else {
			setTimeout("init()", 200);
		}
	}
	function refresh() {
    	redirectPage(document.forms[0].filename.value);
	}
	function initAndStartTimer() {
		if(top.nav.numEntriesConnectedUsers != -1) {
			document.forms[0].numEntries.options[top.nav.numEntriesConnectedUsers].selected = true;
		}
		if(top.nav.refreshIntervalConnectedUsers != -1) {
			document.forms[0].refreshInterval.options[top.nav.refreshIntervalConnectedUsers].selected = true;
		}
		if(top.nav.sortOrderConnectedUsers != -1) {
			document.forms[0].sortOrder.options[top.nav.sortOrderConnectedUsers].selected = true;
		}	
		top.nav.filenameConnectedUsers = setUrl();
		startTimer();
		init();
	}