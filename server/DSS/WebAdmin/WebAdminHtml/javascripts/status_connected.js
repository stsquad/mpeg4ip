// status_connected.js
  function updateForm() {
		document.forms[0].submit();
	}
	function doNothing() {
	}
	function redirectPage(file) {
		var entriesInt, refreshInt, sortInt;
		entriesInt = document.forms[0].numEntries.selectedIndex;
		refreshInt = document.forms[0].refreshInterval.selectedIndex;
		sortInt = document.forms[0].sortOrder.selectedIndex;
		var num = document.forms[0].numEntries.options[entriesInt].text;
		var ref = document.forms[0].refreshInterval.options[refreshInt].text;
		var order = document.forms[0].sortOrder.options[sortInt].text;
		var url = "parsewithinput.cgi?filename="+file+"&numEntries="+num+"&refreshInterval="+ref+"&sortOrder="+order;
		document.location = url;
	}	
	function startTimer() {
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

	function refresh() {
    	redirectPage(document.forms[0].filename.value);
	}