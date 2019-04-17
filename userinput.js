/*
 * Implement all your JavaScript in this file!
 */
 var display = '';
 var standby = false;


function getTemp() {
	$.getJSON("http://localhost:3001/gettemp",
		(data, status) => {
			display = data.display;
			$("#display").html(display);
		});
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
				$("#display").html(display);
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

