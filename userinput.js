/*
 * Implement all your JavaScript in this file!
 */
 var display = '';
 var high = '';
 var low = '';
 var avg = '';
 var standby = false;
 var connected = 1;
 var highThreshold = 27;
 var lowThreshold = 23;

setInterval(getTemp, 1000);

function getTemp() {
	if (standby == false) {
		$.getJSON("http://localhost:3001/gettemp",
			(data, status) => {
				display = data.display;
				high = data.high;
				low = data.low;
				avg = data.avg;
				connected = data.connected;
				console.log(connected);
				setDisplay();
			});
		console.log("gettemp");
	}
}

function reset() {
	getTemp();
	standby = false;
}

function setDisplay() {
	if (connected == 1) {
		$("#display").html(display);
		$("#high").html(high);
		$("#low").html(low);
		$("#avg").html(avg);
	} else {
		$("#display").html(display);
		$("#high").html(high);
		$("#low").html(low);
		$("#avg").html(avg);
	}
}


/*Actions:*/
reset();

/*Button clicks:*/

$("#unit").click(() => {
	if (standby == false) {
		$.getJSON("http://localhost:3001/convert", 
			(data, status) => {
				display = data.display;
				high = data.high;
				low = data.low;
				avg = data.avg;
				setDisplay();
			});
		console.log("units clicked");
	} else {
		console.log("units clicked but on standby");
	}
});

$("#standby").click(() => {
	if (connected == 1) { 
		$.getJSON("http://localhost:3001/standby",
			(data, status) => {
				if (standby == true) {
					reset();
				} else {
					standby = true;
					$("#display").html("standby");
				}
			});
		console.log("standby clicked");
	} else {
		console.log("standby clicked but disconnected");
	}
});

$("#msg").click(() => {
	if (connected == 1 && standby == false) { 
		$.getJSON("http://localhost:3001/sendmsg",
			(data, status) => {
			});
		console.log("send message");
	} else {
		console.log("msg clicked but disconnected or standby");
	}
});

$("#highButton").click(() => {
	if ($("#highInput").val() > lowThreshold && $("#highInput").val() != '') {
		highThreshold = $("#highInput").val();
		var uri = "http://localhost:3001/threshold?high=" + highThreshold + "&low=" + lowThreshold;
		console.log(uri);
		if (standby == false && connected ==1) {
			$.getJSON(uri, 
				(data, status) => {
					// do nothing
				});
			console.log("highButton clicked");
		} else {
			console.log("highButton clicked but on standby or disconnected");
		}
	} else {
		console.log("error, high threshold should be larger than low threshold");
	}
});

$("#lowButton").click(() => {
	if ($("#lowInput").val() < highThreshold && $("#lowInput").val() != '') {
		lowThreshold = $("#lowInput").val();
		var uri = "http://localhost:3001/threshold?high=" + highThreshold + "&low=" + lowThreshold;
		console.log(uri);
		if (standby == false && connected ==1) {
			$.getJSON(uri, 
				(data, status) => {
					// do nothing
				});
			console.log("lowButton clicked");
		} else {
			console.log("lowButton clicked but on standby or disconnected");
		}
	} else {
		console.log("error, low threshold should be smaller than high threshold");
	}
});