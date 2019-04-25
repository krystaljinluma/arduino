/*
 * Implement all your JavaScript in this file!
 */
 var display = '';
 var high = '';
 var low = '';
 var avg = '';
 var standby = false;
 var connected = 1;

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

$("#updatethreshold").click(() => {
	var x = document.getElementById("thresholdinput").value;
 /* need to send x to server*/
});

