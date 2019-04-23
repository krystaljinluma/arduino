/*
 * Implement all your JavaScript in this file!
 */
 var display = '';
 var high = '';
 var low = '';
 var avg = '';
 var standby = false;
 var connected = 0;

setInterval(getTemp, 1000);

function getTemp() {
	$.getJSON("http://localhost:3001/gettemp",
		(data, status) => {
			display = data.display;
			high = data.high;
			low = data.low;
			avg = data.avg;
			connected = data.connected;
			$("#display").html(display);
			$("#high").html(high);
			$("#low").html(low);
			$("#avg").html(avg);
		});
	console.log("gettemp");
}

function reset() {
	getTemp();
	standby = false;
}

/*Actions:*/
reset();

/*Button clicks:*/
$("#update").click(() => {
	if (standby == false) {
		getTemp();
		console.log("update clicked");
	} else {
		console.log("update clicked but on standby");
	}
});

$("#unit").click(() => {
	if (standby == false) {
		$.getJSON("http://localhost:3001/convert", 
			(data, status) => {
				display = data.display;
				high = data.high;
				low = data.low;
				avg = data.avg;
				$("#display").html(display);
				$("#high").html(high);
				$("#low").html(low);
				$("#avg").html(avg);
			});
		console.log("units clicked");
	} else {
		console.log("units clicked but on standby");
	}
});

$("#standby").click(() => {
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
});

