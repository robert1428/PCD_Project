var net = require("net");

var client = new net.Socket();

client.connect(18081, "127.0.0.1", function () {
  console.log("Connected...");

  console.log("Aleg optiunea 1 (arata locatii disponibile)");
  client.write("1\n");
});

client.on("data", function (data) {
  console.log("Received: " + data);

  client.destroy(); // kill client after server's response
});

client.on("close", function () {
  console.log("Connection closed");
});
